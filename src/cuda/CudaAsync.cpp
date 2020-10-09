
// TODO[cdw]: Proper citation of adaptation from work in APEX (Kevin Huck)

#include <stdio.h>
#include <cuda.h>
#include <cupti.h>

#include <iostream>
#include <memory>
#include <stack>
#include <unordered_map>
#include <mutex>
#include <atomic>

#include "apollo/Apollo.h"


static void __attribute__((constructor)) initTrace(void);
//static void __attribute__((destructor)) flushTrace(void);

#define CUPTI_CALL(call)                                                         \
    do {                                                                         \
        CUptiResult _status = call;                                              \
        if (_status != CUPTI_SUCCESS) {                                          \
            const char *errstr;                                                  \
            cuptiGetResultString(_status, &errstr);                              \
            fprintf(stderr, "%s:%d: error: function %s failed with error %s.\n", \
                    __FILE__, __LINE__, #call, errstr);                          \
            exit(-1);                                                            \
        }                                                                        \
    } while (0);

// NOTE[cdw]: Buffer size can be shrunk to force the runtime to flush more often.
#define BUF_SIZE (32 * 1024)
#define ALIGN_SIZE (8)
#define ALIGN_BUFFER(buffer, align)                                            \
    (((uintptr_t) (buffer) & ((align)-1)) ? ((buffer) + (align) - ((uintptr_t) (buffer) & ((align)-1))) : (buffer))


// The callback subscriber
CUpti_SubscriberHandle subscriber;

// The buffer count
std::atomic<uint64_t> num_buffers{0};
std::atomic<uint64_t> num_buffers_processed{0};
bool flushing{false};

void
store_profiler_data(
        const std::string &name,
        uint32_t correlationId,
        uint64_t start,
        uint64_t end,
        uint32_t device,
        uint32_t context,
        uint32_t stream)
{

    static Apollo::Exec* apollo = Apollo::Exec::instance();

    Apollo::Region *reg;

    if (correlationId > 0) {
        reg = apollo->perf.find(correlationId);
        apollo->perf.unbind(correlationId);
    }

    // NOTE[cdw]: Here is where we bring the information over to Apollo.

}


//array enumerating CUpti_OpenAccEventKind strings
const char* openacc_event_names[] = {
    "OpenACC invalid event", // "CUPTI_OPENACC_EVENT_KIND_INVALD",
    "OpenACC device init", // "CUPTI_OPENACC_EVENT_KIND_DEVICE_INIT",
    "OpenACC device shutdown", // "CUPTI_OPENACC_EVENT_KIND_DEVICE_SHUTDOWN",
    "OpenACC runtime shutdown", // "CUPTI_OPENACC_EVENT_KIND_RUNTIME_SHUTDOWN",
    "OpenACC enqueue launch", // "CUPTI_OPENACC_EVENT_KIND_ENQUEUE_LAUNCH",
    "OpenACC enqueue upload", // "CUPTI_OPENACC_EVENT_KIND_ENQUEUE_UPLOAD",
    "OpenACC enqueue download", // "CUPTI_OPENACC_EVENT_KIND_ENQUEUE_DOWNLOAD",
    "OpenACC wait", // "CUPTI_OPENACC_EVENT_KIND_WAIT",
    "OpenACC implicit wait", // "CUPTI_OPENACC_EVENT_KIND_IMPLICIT_WAIT",
    "OpenACC compute construct", // "CUPTI_OPENACC_EVENT_KIND_COMPUTE_CONSTRUCT",
    "OpenACC update", // "CUPTI_OPENACC_EVENT_KIND_UPDATE",
    "OpenACC enter data", // "CUPTI_OPENACC_EVENT_KIND_ENTER_DATA",
    "OpenACC exit data", // "CUPTI_OPENACC_EVENT_KIND_EXIT_DATA",
    "OpenACC create", // "CUPTI_OPENACC_EVENT_KIND_CREATE",
    "OpenACC delete", // "CUPTI_OPENACC_EVENT_KIND_DELETE",
    "OpenACC alloc", // "CUPTI_OPENACC_EVENT_KIND_ALLOC",
    "OpenACC free" // "CUPTI_OPENACC_EVENT_KIND_FREE"
};

const char* openmp_event_names[] = {
    "OpenMP Invalid", // CUPTI_OPENMP_EVENT_KIND_INVALID
    "OpenMP Parallel", // CUPTI_OPENMP_EVENT_KIND_PARALLEL
    "OpenMP Task", // CUPTI_OPENMP_EVENT_KIND_TASK
    "OpenMP Thread", // CUPTI_OPENMP_EVENT_KIND_THREAD
    "OpenMP Idle", // CUPTI_OPENMP_EVENT_KIND_IDLE
    "OpenMP Wait Barrier", // CUPTI_OPENMP_EVENT_KIND_WAIT_BARRIER
    "OpenMP Wait Taskwait" // CUPTI_OPENMP_EVENT_KIND_WAIT_TASKWAIT
};

static void
kernelActivity(CUpti_Activity *record)
{
    CUpti_ActivityKernel4 *kernel =
        (CUpti_ActivityKernel4 *) record;
    std::string tmp = std::string(kernel->name);
    store_profiler_data(tmp, kernel->correlationId, kernel->start,
            kernel->end, kernel->deviceId, kernel->contextId,
            kernel->streamId);
}

static void openaccDataActivity(CUpti_Activity *record) {
    CUpti_ActivityOpenAccData *data = (CUpti_ActivityOpenAccData *) record;
    std::string label{openacc_event_names[data->eventKind]};
    store_profiler_data(label, data->externalId, data->start,
        data->end, data->cuDeviceId, data->cuContextId, data->cuStreamId);
    static std::string bytes{"Bytes Transferred"};
    //store_counter_data(label.c_str(), bytes, data->end, data->bytes);
}

static void openaccKernelActivity(CUpti_Activity *record) {
    CUpti_ActivityOpenAccLaunch *data = (CUpti_ActivityOpenAccLaunch *) record;
    std::string label{openacc_event_names[data->eventKind]};
    store_profiler_data(label, data->externalId, data->start,
            data->end, data->cuDeviceId, data->cuContextId,
            data->cuStreamId);
    static std::string gangs{"Num Gangs"};
    //store_counter_data(label.c_str(), gangs, data->end, data->numGangs);
    static std::string workers{"Num Workers"};
    //store_counter_data(label.c_str(), workers, data->end, data->numWorkers);
    static std::string lanes{"Num Vector Lanes"};
    //store_counter_data(label.c_str(), lanes, data->end, data->vectorLength);
}

static void openaccOtherActivity(CUpti_Activity *record) {
    CUpti_ActivityOpenAccOther *data = (CUpti_ActivityOpenAccOther *) record;
    std::string label{openacc_event_names[data->eventKind]};
    store_profiler_data(label, data->externalId, data->start,
            data->end, data->cuDeviceId, data->cuContextId,
            data->cuStreamId);
}

static void openmpActivity(CUpti_Activity *record) {
    CUpti_ActivityOpenMp *data = (CUpti_ActivityOpenMp *) record;
    std::string label{openmp_event_names[data->eventKind]};
}

static void printActivity(CUpti_Activity *record) {
    switch (record->kind)
    {
#if 0
        case CUPTI_ACTIVITY_KIND_DEVICE: {
            deviceActivity(record);
            break;
        }
        case CUPTI_ACTIVITY_KIND_DEVICE_ATTRIBUTE: {
            deviceAttributeActivity(record);
            break;
        }
        case CUPTI_ACTIVITY_KIND_CONTEXT: {
            contextActivity(record);
            break;
        }
        case CUPTI_ACTIVITY_KIND_MEMCPY:
        {
            memoryActivity(record);
            break;
        }
        case CUPTI_ACTIVITY_KIND_MEMCPY2:
        {
            memoryActivity2(record);
            break;
        }
        case CUPTI_ACTIVITY_KIND_UNIFIED_MEMORY_COUNTER: {
            unifiedMemoryActivity(record);
            break;
        }
        case CUPTI_ACTIVITY_KIND_MEMCPY2:
        {
            // NOTE: Requires CUDA 11
            CUpti_ActivityMemcpyPtoP *memcpy = (CUpti_ActivityMemcpyPtoP *) record;
            break;
        }
        case CUPTI_ACTIVITY_KIND_MEMSET: {
            memsetActivity(record);
            break;
        }
#endif
        case CUPTI_ACTIVITY_KIND_KERNEL:
        case CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL:
        case CUPTI_ACTIVITY_KIND_CDP_KERNEL:
        {
            kernelActivity(record);
            break;
        }
        case CUPTI_ACTIVITY_KIND_OPENACC_DATA: {
            openaccDataActivity(record);
            break;
        }
        case CUPTI_ACTIVITY_KIND_OPENACC_LAUNCH: {
            openaccKernelActivity(record);
            break;
        }
        case CUPTI_ACTIVITY_KIND_OPENACC_OTHER: {
            openaccOtherActivity(record);
            break;
        }
        case CUPTI_ACTIVITY_KIND_OPENMP: {
            openmpActivity(record);
            break;
        }
        default:
#if 0
            printf("  <unknown>\n");
#endif
            break;
    }
}

void CUPTIAPI bufferRequested(uint8_t **buffer, size_t *size,
    size_t *maxNumRecords)
{
    num_buffers++;
    uint8_t *bfr = (uint8_t *) malloc(BUF_SIZE + ALIGN_SIZE);
    if (bfr == NULL) {
        printf("Error: out of memory\n");
        exit(-1);
    }

    *size = BUF_SIZE;
    *buffer = ALIGN_BUFFER(bfr, ALIGN_SIZE);
    *maxNumRecords = 0;
}

void CUPTIAPI bufferCompleted(CUcontext ctx, uint32_t streamId,
    uint8_t *buffer, size_t size, size_t validSize)
{
    num_buffers_processed++;
    if (flushing) { std::cout << "." << std::flush; }
    CUptiResult status;
    CUpti_Activity *record = NULL;

    if (validSize > 0) {
        do {
            status = cuptiActivityGetNextRecord(buffer, validSize, &record);
            if (status == CUPTI_SUCCESS) {
                printActivity(record);
            }
            else if (status == CUPTI_ERROR_MAX_LIMIT_REACHED)
                break;
            else {
                CUPTI_CALL(status);
            }
        } while (1);

        // report any records dropped from the queue
        size_t dropped;
        CUPTI_CALL(cuptiActivityGetNumDroppedRecords(ctx, streamId, &dropped));
        if (dropped != 0) {
            printf("== APOLLO: Dropped %u CUDA activity records.\n", (unsigned int) dropped);
        }
    }

    free(buffer);
}


bool initialize_first_time() {
    // ...
    return true;
}


void register_new_context(const void *params) {
#if 0
    CUpti_ResourceData * rd = (CUpti_ResourceData*)(params);
    // Register for async activity ON THIS CONTEXT!
    CUPTI_CALL(cuptiActivityEnableContext(rd->context,
        CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL)); // 10
    //CUPTI_CALL(cuptiActivityEnableContext(rd->context,
    //    CUPTI_ACTIVITY_KIND_MEMCPY)); // 1
    //CUPTI_CALL(cuptiActivityEnableContext(rd->context,
    //    CUPTI_ACTIVITY_KIND_MEMCPY2)); // 22
    //CUPTI_CALL(cuptiActivityEnableContext(rd->context,
    //    CUPTI_ACTIVITY_KIND_MEMSET)); // 2
#else
    CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL)); // 10
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_MEMCPY)); // 1
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_MEMCPY2)); // 22
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_MEMSET)); // 2
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_OPENACC_DATA)); // 33
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_OPENACC_LAUNCH)); // 34
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_OPENACC_OTHER)); // 35
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_OPENMP)); // 47
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_KERNEL)); // 3   <- disables concurrency
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_DRIVER)); // 4
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_RUNTIME)); // 5
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_EVENT)); // 6
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_METRIC)); // 7
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_DEVICE)); // 8
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_CONTEXT)); // 9
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_NAME)); // 11
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_MARKER)); // 12
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_MARKER_DATA)); // 13
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_SOURCE_LOCATOR)); // 14
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_GLOBAL_ACCESS)); // 15
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_BRANCH)); // 16
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_OVERHEAD)); // 17
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_CDP_KERNEL)); // 18
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_PREEMPTION)); // 19
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_ENVIRONMENT)); // 20
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_EVENT_INSTANCE)); // 21
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_METRIC_INSTANCE)); // 23
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_INSTRUCTION_EXECUTION)); // 24
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_UNIFIED_MEMORY_COUNTER)); // 25
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_FUNCTION)); // 26
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_MODULE)); // 27
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_DEVICE_ATTRIBUTE)); // 28
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_SHARED_ACCESS)); // 29
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_PC_SAMPLING)); // 30
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_PC_SAMPLING_RECORD_INFO)); // 31
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_INSTRUCTION_CORRELATION)); // 32
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_CUDA_EVENT)); // 36
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_STREAM)); // 37
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_SYNCHRONIZATION)); // 38
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_EXTERNAL_CORRELATION)); // 39
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_NVLINK)); // 40
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_INSTANTANEOUS_EVENT)); // 41
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_INSTANTANEOUS_EVENT_INSTANCE)); // 42
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_INSTANTANEOUS_METRIC)); // 43
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_INSTANTANEOUS_METRIC_INSTANCE)); // 44
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_MEMORY)); // 45
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_PCIE)); // 46
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_INTERNAL_LAUNCH_API)); // 48
    //CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_COUNT)); // 49
#endif
}

void apollo_cupti_callback_dispatch(void *ud, CUpti_CallbackDomain domain,
        CUpti_CallbackId id, const void *params) {
    static bool initialized = initialize_first_time();

    if (params == NULL) { return; }

    if (domain == CUPTI_CB_DOMAIN_RESOURCE &&
        id == CUPTI_CBID_RESOURCE_CONTEXT_CREATED) {
        // Here we tell CUPTI what data we're interested in.
        register_new_context(params);
        return;
    }

    //CUpti_CallbackData *cbdata = (CUpti_CallbackData*)(params);
}

void initTrace() {

    // Register callbacks for buffer requests and for buffers completed by CUPTI.
    CUPTI_CALL(cuptiActivityRegisterCallbacks(bufferRequested, bufferCompleted));

    // Get and set activity attributes.
    // Attributes can be set by the CUPTI client to change behavior of the activity API.
    // Some attributes require to be set before any CUDA context is created to be effective,
    // e.g. to be applied to all device buffer allocations (see documentation).

    size_t attrValue = 0, attrValueSize = sizeof(size_t);
    CUPTI_CALL(cuptiActivityGetAttribute(
                CUPTI_ACTIVITY_ATTR_DEVICE_BUFFER_SIZE, &attrValueSize, &attrValue));
    attrValue = attrValue / 4;
    CUPTI_CALL(cuptiActivitySetAttribute(
                CUPTI_ACTIVITY_ATTR_DEVICE_BUFFER_SIZE, &attrValueSize, &attrValue));

    CUPTI_CALL(cuptiActivityGetAttribute(
                CUPTI_ACTIVITY_ATTR_DEVICE_BUFFER_POOL_LIMIT, &attrValueSize, &attrValue));
    attrValue = attrValue * 4;
    CUPTI_CALL(cuptiActivitySetAttribute(
                CUPTI_ACTIVITY_ATTR_DEVICE_BUFFER_POOL_LIMIT, &attrValueSize, &attrValue));

    // ** ENABLED for DISPATCH ** Uncomment to subscribe to callback support, too.
    //CUPTI_CALL(cuptiSubscribe(&subscriber,
    //            (CUpti_CallbackFunc)apollo_cupti_callback_dispatch, NULL));

    // ** Activate specific domains of CUPTI events:
    CUPTI_CALL(cuptiEnableDomain(1, subscriber, CUPTI_CB_DOMAIN_RUNTIME_API));
    CUPTI_CALL(cuptiEnableDomain(1, subscriber, CUPTI_CB_DOMAIN_DRIVER_API));

    // ** DISABLED for DISPATCH **
    CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL)); // 10

    // NOTE[cdw]: Uncomment to see CUPTI_CBID_RESOURCE_CONTEXT_CREATED events.
    //CUPTI_CALL(cuptiEnableDomain(1, subscriber, CUPTI_CB_DOMAIN_RESOURCE));

    // These events aren't begin/end callbacks, so no need to support them.
    //CUPTI_CALL(cuptiEnableDomain(1, subscriber, CUPTI_CB_DOMAIN_SYNCHRONIZE));
    //CUPTI_CALL(cuptiEnableDomain(1, subscriber, CUPTI_CB_DOMAIN_NVTX));

}

namespace Apollo {

void
flushTrace(void)
{
    Apollo::Exec *apollo = Apollo::Exec::instance();

    if ((num_buffers_processed + 10) < num_buffers) {
        if (apollo->env.mpi_rank == 0) {
            //flushing = true;
            std::cout << "Flushing remaining " << std::fixed
                << num_buffers-num_buffers_processed << " of " << num_buffers
                << " CUDA/CUPTI buffers..." << std::endl;
        }
    }
    cuptiActivityFlushAll(CUPTI_ACTIVITY_FLAG_NONE);
    if (flushing) {
        std::cout << std::endl;
    }
}
}



