#ifndef PIPELINEWORKER_H
#define PIPELINEWORKER_H

#include <QObject>

#include "workerconfig.h"

class PipelineWorker : public QObject
{
    Q_OBJECT

  public:
    enum class State
    {
        Uninitialized,
        Running,
        Stopped
    };

    explicit PipelineWorker(QObject* parent = nullptr);
    ~PipelineWorker() override;

    State state() const;

  public slots:
    virtual void configure(const WorkerConfigPtr& config);

  protected:
    virtual void run() = 0;
    virtual void flush() = 0;
    virtual void release() = 0;
    virtual void stop() = 0;

  protected:
    State m_state;
};

#endif // PIPELINEWORKER_H
