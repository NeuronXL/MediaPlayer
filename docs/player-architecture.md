# MediaPlayer 架构设计说明（可演进到视频/音频/字幕）

## 1. 类结构图

```text
View / ViewModel (主线程)
        │
        ▼
MediaPlayerEngine
  - 播放状态机
  - 播放时钟与 A/V 同步
  - 命令编排（open/play/pause/seek/stop）
        │ (Queued Signal/Slot)
        ▼
MediaPipelineService
  - 媒体管线门面与生命周期管理
  ├─ FFmpegDemuxer
  │    - av_read_frame 读包
  │    - 按 stream_index 分发到多轨
  ├─ FFmpegVideoDecoder
  │    - 视频包 -> VideoFrame
  ├─ FFmpegAudioDecoder
  │    - 音频包 -> AudioFrame
  ├─ FFmpegSubtitleDecoder
  │    - 字幕包 -> SubtitleFrame
  ├─ PacketQueue(video/audio/subtitle)
  └─ FrameQueue(video/audio/subtitle)
        │
        ▼
Output Adapters
  ├─ VideoRendererAdapter      (VideoFrame -> QImage/QVideoFrame)
  ├─ AudioOutputAdapter        (AudioFrame -> QAudioSink 输入)
  └─ SubtitleRendererAdapter   (SubtitleFrame -> UI Overlay)
```

---

## 2. 分层与对象职责

| 层 | 对象 | 主要职责 | 不应承担 |
|---|---|---|---|
| UI 层 | `MainWindow / PlaybackControlWidget` | 展示画面、播放控制、进度条、字幕叠加；收集用户操作 | 不处理 FFmpeg 资源与播放时序 |
| UI 层 | `MainWindowViewModel` | 把 UI 操作映射为 Engine 命令；把 Engine 状态转为 UI 可绑定数据 | 不持有 FFmpeg 对象，不做解码 |
| 业务编排层 | `MediaPlayerEngine` | 播放状态机；播放时钟；A/V 同步；seek/play/pause/stop 协调 | 不直接调用 `av_read_frame/avcodec_send_packet/avcodec_receive_frame` |
| 媒体管线层 | `MediaPipelineService` | 作为管线门面管理 Demuxer/Decoder/队列生命周期；统一对外输出帧流 | 不做播放状态决策 |
| 媒体管线层 | `FFmpegDemuxer` | 从容器读 `AVPacket` 并按流类型分发到各轨包队列 | 不做播放时钟控制 |
| 媒体管线层 | `FFmpegVideoDecoder` | 消费视频包并产出 `VideoFrame` | 不做 UI 渲染与业务状态机 |
| 媒体管线层 | `FFmpegAudioDecoder` | 消费音频包并产出 `AudioFrame`（PCM+格式） | 不做 A/V 主时钟决策 |
| 媒体管线层 | `FFmpegSubtitleDecoder` | 消费字幕包并产出 `SubtitleFrame` | 不做字幕绘制 |
| 缓冲层 | `PacketQueue` | 缓冲压缩包，做容量控制和背压 | 不做业务状态判断 |
| 缓冲层 | `FrameQueue` | 缓冲解码帧，供 Engine 按时钟消费 | 不做时钟策略 |
| 输出适配层 | `VideoRendererAdapter` | `VideoFrame -> QImage/QVideoFrame` | 不控制播放状态 |
| 输出适配层 | `AudioOutputAdapter` | `AudioFrame -> 音频设备（如 QAudioSink）` | 不做业务状态机 |
| 输出适配层 | `SubtitleRendererAdapter` | `SubtitleFrame -> UI Overlay` | 不做解码/时钟主控 |

---

## 3. 关键数据对象

```cpp
enum class TrackType {
    Video,
    Audio,
    Subtitle
};

struct MediaPacket {
    TrackType track;
    AVPacket* pkt;
};

struct AudioFormat {
    int sampleRate = 0;
    int channels = 0;
    int sampleFormat = 0; // 对应 AVSampleFormat 或项目内部枚举
    qint64 channelLayout = 0;
};

struct VideoFrame {
    qint64 ptsMs = 0;
    qint64 durMs = 0;
    bool key = false;
    QImage image;
};

struct AudioFrame {
    qint64 ptsMs = 0;
    qint64 durMs = 0;
    QByteArray pcm;
    AudioFormat fmt;
};

struct SubtitleFrame {
    qint64 startMs = 0;
    qint64 endMs = 0;
    QString text;
};

struct TrackInfo {
    int streamIndex = -1;
    TrackType track = TrackType::Video;
    QString codecName;
    QString language;
};

struct MediaInfo {
    QString filePath;
    qint64 durationMs = 0;
    QList<TrackInfo> tracks;
};
```

---

## 4. 关键数据对象字段含义

| 对象 | 字段 | 含义 |
|---|---|---|
| `MediaPacket` | `track` | 当前包所属轨道类型（视频/音频/字幕） |
| `MediaPacket` | `pkt` | FFmpeg 压缩包指针，需明确 `unref/free` 所有权 |
| `AudioFormat` | `sampleRate` | 采样率（Hz），如 `44100/48000` |
| `AudioFormat` | `channels` | 声道数，如 `1/2/6` |
| `AudioFormat` | `sampleFormat` | 采样格式（如 `s16/fltp`） |
| `AudioFormat` | `channelLayout` | 声道布局（单声道/立体声/5.1） |
| `VideoFrame` | `ptsMs` | 显示时间戳（毫秒），用于视频调度和进度更新 |
| `VideoFrame` | `durMs` | 帧持续时长（毫秒），用于调度兜底 |
| `VideoFrame` | `key` | 是否关键帧，用于 seek/丢帧策略 |
| `VideoFrame` | `image` | 可直接渲染的图像数据 |
| `AudioFrame` | `ptsMs` | 音频帧时间戳（毫秒），常作为主时钟参考 |
| `AudioFrame` | `durMs` | 音频帧覆盖时长（毫秒），用于播放位置推进 |
| `AudioFrame` | `pcm` | 解码后的原始 PCM 字节数据 |
| `AudioFrame` | `fmt` | PCM 格式描述，供输出层正确消费 |
| `SubtitleFrame` | `startMs` | 字幕开始显示时间（毫秒） |
| `SubtitleFrame` | `endMs` | 字幕结束显示时间（毫秒） |
| `SubtitleFrame` | `text` | 文本字幕内容（位图字幕可扩展图像字段） |
| `TrackInfo` | `streamIndex` | 容器内流索引（FFmpeg stream index） |
| `TrackInfo` | `track` | 轨道类型（视频/音频/字幕） |
| `TrackInfo` | `codecName` | 该流使用的编解码器名称 |
| `TrackInfo` | `language` | 轨道语言（若容器 metadata 提供） |
| `MediaInfo` | `filePath` | 当前打开媒体文件路径 |
| `MediaInfo` | `durationMs` | 媒体总时长（毫秒） |
| `MediaInfo` | `tracks` | 轨道信息集合（用于轨道选择和信息展示） |

---

## 5. 设计约束（推荐）

1. 对外统一使用毫秒时间单位（`ms`），避免多 time_base 泄漏到上层。  
2. `MediaPlayerEngine` 只消费标准帧对象，不直接持有 FFmpeg 上下文。  
3. `MediaPipelineService` 只做生产与资源管理，不做 UI 状态决策。  
4. 音频接入后，以音频时钟为主，视频和字幕跟随音频时钟同步。  
