#include "cm_engine.hpp"

using namespace CyanMycelium;

InferenceSessionPtr InferenceEngine ::CreateSession(GraphPtr model, IMemoryManagerPtr memoryManager)
{
  return new InferenceSession(model, &_queue, memoryManager);
}

void InferenceEngine ::Start()
{
  // we create the thread
  _lock.Take();
  if (!_started)
  {
    _started = true;
    _threads = new ThreadPtr[_options.ThreadCount];
    IRunnable *runnable = _options.Runtime ? _options.Runtime : this;
    for (int i = 0; i < _options.ThreadCount; i++)
    {
      _threads[i] = new Thread(runnable, _options.StackSize, this, _options.Priority);
    }
  }
  _lock.Give();
}

void InferenceEngine ::Stop()
{
  _started = false;
}

bool InferenceEngine ::IsStarted()
{
  _lock.Take();
  bool tmp = _started;
  _lock.Give();
  return tmp;
}

unsigned long InferenceEngine ::Run(void *)
{
  if (IsStarted())
  {
    ActivationEvent e;
    do
    {
      if (_queue.Receive(&e, _options.WaitTimeout))
      {
        Consume(e);
      }
    } while (IsStarted());
  }
  return 0l;
}

void InferenceEngine ::Consume(ActivationEvent &e)
{
  IActivationCtx *context = e.Context;

  switch (e.Type)
  {
  case CM_ACTIVATION_BEGIN:
  {
    context->OnBegin((KeyValueCollection<void *> *)e.Content);
    break;
  }
  case CM_ACTIVATION_NODE:
  {
    NodePtr node = (NodePtr)e.Content;
    node->Activate(context);
    int count = node->Onsc.Count();
    for (int i = 0; i != count; ++i)
    {
      context->Activate(node->Onsc[i]);
    }
    break;
  }
  case CM_ACTIVATION_LINK:
  {
    break;
  }
  case CM_ACTIVATION_END:
  {
    context->OnEnd((KeyValueCollection<void *> *)e.Content);
    break;
  }
  }
}
