# PooledCalculation

This project provides a library implementation that can be used for distributed computing.
There are two different possibilities of achieving this:

1. Single-machine execution (see `ThreadPool` class)
2. Multi-machine execution (see `ServerPool` class)

With `ThreadPool` function can be directly scheduled in a thread pool and the results of the
functions are available via a `std::future` object.
Conversely, when using the `ServerPool` functionality, the `ServerPoolWorker` class has to be 
derived from and the `DoWork` function must be defined. The data is passed between the master 
and the worker servers via serialized data packages (see `DataPackagePayload`). The resulting
data package is, again, available via a `std::future` object. Alternatively, the `ServerPool` 
class defines a `OnTaskCompleted` method that can be overwritten.

