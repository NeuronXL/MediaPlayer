#include "pipelineworker.h"

PipelineWorker::PipelineWorker(QObject* parent)
    : QObject(parent)
    , m_state(State::Uninitialized)
{
}

PipelineWorker::~PipelineWorker() = default;

PipelineWorker::State PipelineWorker::state() const
{
    return m_state;
}

void PipelineWorker::configure(const WorkerConfigPtr& config)
{
    
}

