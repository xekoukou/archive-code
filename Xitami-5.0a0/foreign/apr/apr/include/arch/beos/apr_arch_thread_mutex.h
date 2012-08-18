/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef THREAD_MUTEX_H
#define THREAD_MUTEX_H

#include <kernel/OS.h>
#include "apr_pools.h"
#include "apr_thread_mutex.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_portable.h"

struct apr_thread_mutex_t {
    apr_pool_t *pool;
    
    /* Our lock :) */
    sem_id Lock;
    int32  LockCount;

    /* If we nest locks we need these... */
    int nested;  
    apr_os_thread_t owner;
    int owner_ref;
};

#endif  /* THREAD_MUTEX_H */

