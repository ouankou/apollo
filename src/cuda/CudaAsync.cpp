

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


static void
kernelActivity(CUpti_Activity *record)
{
    CUpti_ActivityKernel4 *kernel = (CUpti_ActivityKernel4 *) record;
    static Apollo::Exec   *apollo = Apollo::Exec::instance();

    // kernel->correlationId  (uint32_t)
    // kernel->start          (uint64_t)
    // kernel->end            (uint64_t)
    // kernel->blockX         (uint32_t)
    // kernel->blockY         (uint32_t)
    // kernel->blockZ         (uint32_t)
    // kernel->deviceId       (uint32_t)
    // kernel->contextId      (uint32_t)
    // kernel->streamId       (uint32_t)

    Apollo::Perf::Record *pr = nullptr;

    if (kernel->correlationId > 0) {
        pr = apollo->perf.find(correlationId);
        apollo->perf.unbind(correlationId);
    }

    // NOTE[cdw]: I need to have a customized cudaLaunchKernel() call over on
    //            the RAJA side, which creates a performance record and drops
    //            it into the stream queue.

    // NOTE[cdw]: Here is where we bring the information over to Apollo.

    // When the buffer arrives the kernel has already finished. Lookup here.
    // Extract ApolloContext from the map using the correlationID
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
                kernelActivity(record);
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


void apollo_cupti_callback_dispatch(void *ud, CUpti_CallbackDomain domain,
        CUpti_CallbackId id, const void *params) {
    static bool initialized = initialize_first_time();

    if (params == NULL) { return; }

    CUpti_CallbackData *cbdata = (CUpti_CallbackData*)(params);

    cudaStream_t stream;
    switch (id) {
        case CUPTI_RUNTIME_TRACE_CBID_cudaLaunchKernel_v7000:
        {
            stream = ((cudaLaunchKernel_v7000_params_st*)(params))->stream;
            break;
        }
        case CUPTI_RUNTIME_TRACE_CBID_cudaLaunchKernel_ptsz_v7000:
        {
            stream = ((cudaLaunchKernel_ptsz_v7000_params_st*)(params))->stream;
            break;
        }
        // NOTE[cdw]: We should not be seeing any of these kinds of kernels, so emit warning.
        case CUPTI_RUNTIME_TRACE_CBID_cudaLaunchCooperativeKernel_v9000:
        case CUPTI_RUNTIME_TRACE_CBID_cudaLaunchCooperativeKernel_ptsz_v9000:
        case CUPTI_RUNTIME_TRACE_CBID_cudaLaunchCooperativeKernelMultiDevice_v9000:
        case CUPTI_RUNTIME_TRACE_CBID_cudaGraphKernelNodeGetParams_v10000:
        case CUPTI_RUNTIME_TRACE_CBID_cudaGraphKernelNodeSetParams_v10000:
        case CUPTI_RUNTIME_TRACE_CBID_cudaGraphAddKernelNode_v10000:
        case CUPTI_RUNTIME_TRACE_CBID_cudaGraphExecKernelNodeSetParams_v10010:
        default:
            //TODO[cdw]: Add support for LLNL/other versions of CUDA if needed.
            break;
    }

    //You can then cast cudaStream_t to CUstream and use it in the cuptiGetStreamId() call.
    //TODO[cdw]: Lookup the CUDA stream for this kernel launch.
    //TODO[cdw]: Migrate the ApolloContext from queue keyed by streamId into
    //           a map keyed by cbdata->correlationId.


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
    CUPTI_CALL(cuptiSubscribe(&subscriber,
                (CUpti_CallbackFunc)apollo_cupti_callback_dispatch, NULL));

    // ** Activate specific domains of CUPTI events:
    // RUNTIME =cuda* calls     DRIVER =cu* (low level API)
    CUPTI_CALL(cuptiEnableDomain(1, subscriber, CUPTI_CB_DOMAIN_RUNTIME_API));
    //CUPTI_CALL(cuptiEnableDomain(1, subscriber, CUPTI_CB_DOMAIN_DRIVER_API));

    // Register events we are interested in:
    CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL)); // 10

    return;
}

namespace Apollo {

void
flushTrace(void)
{
    Apollo::Exec *apollo = Apollo::Exec::instance();

    // NOTE: Debugging output...
    // if ((num_buffers_processed + 10) < num_buffers) {
    //     if (apollo->env.mpi_rank == 0) {
    //         //flushing = true;
    //         std::cout << "Flushing remaining " << std::fixed
    //             << num_buffers-num_buffers_processed << " of " << num_buffers
    //             << " CUDA/CUPTI buffers..." << std::endl;
    //     }
    // }

    cuptiActivityFlushAll(CUPTI_ACTIVITY_FLAG_NONE);

    // NOTE: Debugging output...
    // if (flushing) {
    //     std::cout << std::endl;
    // }
}
}



