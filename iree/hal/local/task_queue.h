// Copyright 2020 The IREE Authors
//
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef IREE_HAL_LOCAL_TASK_QUEUE_H_
#define IREE_HAL_LOCAL_TASK_QUEUE_H_

#include <stdint.h>

#include "iree/base/api.h"
#include "iree/base/internal/synchronization.h"
#include "iree/hal/api.h"
#include "iree/hal/local/arena.h"
#include "iree/hal/local/task_queue_state.h"
#include "iree/task/executor.h"
#include "iree/task/scope.h"
#include "iree/task/task.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

typedef struct iree_hal_task_queue_t {
  // Shared executor that the queue submits tasks to.
  iree_task_executor_t* executor;

  // Shared block pool for allocating submission transients (tasks/events/etc).
  iree_arena_block_pool_t* block_pool;

  // Scope used for all tasks in the queue.
  // This allows for easy waits on all outstanding queue tasks as well as
  // differentiation of tasks within the executor.
  iree_task_scope_t scope;

  // Guards queue state. Submissions and waits may come from any user thread and
  // we do a bit of bookkeeping during command buffer issue that will come from
  // an executor thread.
  iree_slim_mutex_t mutex;

  // State tracking used during command buffer issue.
  // The intra-queue synchronization (barriers/events) carries across command
  // buffers and this is used to rendezvous the tasks in each set.
  iree_hal_task_queue_state_t state;

  // The last active iree_hal_task_queue_issue_cmd_t submitted to the queue.
  // If this is NULL then there are no issues pending - though there may still
  // be active work that was previously issued. This is used to chain together
  // issues in FIFO order such that all submissions *issue* in order but not
  // *execute* in order.
  iree_task_t* tail_issue_task;
} iree_hal_task_queue_t;

void iree_hal_task_queue_initialize(iree_string_view_t identifier,
                                    iree_task_executor_t* executor,
                                    iree_arena_block_pool_t* block_pool,
                                    iree_hal_task_queue_t* out_queue);

void iree_hal_task_queue_deinitialize(iree_hal_task_queue_t* queue);

iree_status_t iree_hal_task_queue_submit(
    iree_hal_task_queue_t* queue, iree_host_size_t batch_count,
    const iree_hal_submission_batch_t* batches);

iree_status_t iree_hal_task_queue_submit_and_wait(
    iree_hal_task_queue_t* queue, iree_host_size_t batch_count,
    const iree_hal_submission_batch_t* batches,
    iree_hal_semaphore_t* wait_semaphore, uint64_t wait_value,
    iree_timeout_t timeout);

iree_status_t iree_hal_task_queue_wait_idle(iree_hal_task_queue_t* queue,
                                            iree_timeout_t timeout);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // IREE_HAL_LOCAL_TASK_QUEUE_H_
