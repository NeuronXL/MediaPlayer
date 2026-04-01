#!/usr/bin/env python3
import argparse
import bisect
import re
import subprocess
import tempfile
from pathlib import Path


def run_cmd(cmd):
    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if p.returncode != 0:
        raise RuntimeError(f"Command failed:\n{' '.join(cmd)}\n\nstderr:\n{p.stderr}")
    return p.stdout, p.stderr


def detect_edges(samples, threshold):
    events = []
    bright = False
    for pts, val in samples:
        now_bright = val > threshold
        if now_bright and not bright:
            events.append(pts)
        bright = now_bright
    return dedup_events(events, 0.2)


def extract_video_events(video_path: str, meta_path: str, y_threshold: float, crop: str):
    # 用 signalstats 输出每帧亮度统计到文件
    vf = "signalstats,metadata=print:file={}".format(meta_path)
    if crop:
        vf = "crop={},{}".format(crop, vf)

    cmd = [
        "ffmpeg", "-y",
        "-i", video_path,
        "-vf", vf,
        "-an", "-f", "null", "-"
    ]
    run_cmd(cmd)

    cur_pts = None
    ymax_samples = []
    yavg_samples = []

    with open(meta_path, "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            m_pts = re.search(r"pts_time:([0-9.]+)", line)
            if m_pts:
                cur_pts = float(m_pts.group(1))

            m_ymax = re.search(r"lavfi\.signalstats\.YMAX=([0-9.]+)", line)
            if m_ymax and cur_pts is not None:
                ymax_samples.append((cur_pts, float(m_ymax.group(1))))
            m_yavg = re.search(r"lavfi\.signalstats\.YAVG=([0-9.]+)", line)
            if m_yavg and cur_pts is not None:
                yavg_samples.append((cur_pts, float(m_yavg.group(1))))

    events = detect_edges(ymax_samples, y_threshold)
    if events:
        return events

    if not yavg_samples:
        return []

    yavg_values = [v for _, v in yavg_samples]
    fallback_threshold = max(yavg_values) - 3.0
    return detect_edges(yavg_samples, fallback_threshold)


def dedup_events(events, min_gap_s: float):
    dedup = []
    for t in events:
        if not dedup or t - dedup[-1] > min_gap_s:
            dedup.append(t)
    return dedup


def extract_audio_events(video_path: str, noise_db: str, min_silence: str):
    # silencedetect 日志在 stderr
    cmd = [
        "ffmpeg", "-i", video_path,
        "-af", f"silencedetect=noise={noise_db}:d={min_silence}",
        "-f", "null", "-"
    ]
    _, err = run_cmd(cmd)

    events = []
    for line in err.splitlines():
        m = re.search(r"silence_end:\s*([0-9.]+)", line)
        if m:
            events.append(float(m.group(1)))
    return dedup_events(events, 0.2)


def pair_offsets(video_events, audio_events, max_events):
    if not video_events or not audio_events:
        return []

    deltas = []
    for t in video_events[:max_events]:
        idx = bisect.bisect_left(audio_events, t)
        candidates = []
        if idx < len(audio_events):
            candidates.append((audio_events[idx] - t) * 1000.0)
        if idx > 0:
            candidates.append((audio_events[idx - 1] - t) * 1000.0)
        if candidates:
            deltas.append(min(candidates, key=lambda x: abs(x)))

    if len(deltas) < 5:
        return deltas

    sorted_deltas = sorted(deltas)
    median = sorted_deltas[len(sorted_deltas) // 2]
    filtered = [d for d in deltas if abs(d - median) <= 150.0]
    return filtered if filtered else deltas


def summarize(offsets_ms):
    if not offsets_ms:
        return "未提取到可配对事件，无法计算。"

    avg = sum(offsets_ms) / len(offsets_ms)
    mn = min(offsets_ms)
    mx = max(offsets_ms)

    if abs(avg) <= 40:
        level = "基本可接受"
    elif abs(avg) <= 80:
        level = "可感知偏移，建议优化"
    else:
        level = "偏移明显，需要修正"

    direction = "音频领先视频" if avg > 0 else "视频领先音频"
    return (
        f"样本数: {len(offsets_ms)}\n"
        f"平均偏移: {avg:.2f} ms ({direction})\n"
        f"最小/最大: {mn:.2f} / {mx:.2f} ms\n"
        f"结论: {level}"
    )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("input", help="录制文件路径（mov/mp4 都可）")
    parser.add_argument("--crop", default="", help="可选裁剪区域，格式: w:h:x:y")
    parser.add_argument("--y-threshold", type=float, default=245.0, help="白闪检测阈值(YMAX)")
    parser.add_argument("--noise-db", default="-35dB", help="silencedetect noise")
    parser.add_argument("--min-silence", default="0.02", help="silencedetect d")
    parser.add_argument("--max-events", type=int, default=30, help="最多配对事件数")
    args = parser.parse_args()

    input_path = str(Path(args.input).resolve())
    with tempfile.TemporaryDirectory() as td:
        meta_path = str(Path(td) / "video_meta.txt")

        video_events = extract_video_events(input_path, meta_path, args.y_threshold, args.crop)
        audio_events = extract_audio_events(input_path, args.noise_db, args.min_silence)
        offsets_ms = pair_offsets(video_events, audio_events, args.max_events)

        print("video_events(head):", [round(x, 4) for x in video_events[:10]])
        print("audio_events(head):", [round(x, 4) for x in audio_events[:10]])
        print("offsets_ms(head): ", [round(x, 2) for x in offsets_ms[:10]])
        print()
        print(summarize(offsets_ms))


if __name__ == "__main__":
    main()
