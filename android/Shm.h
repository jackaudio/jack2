/*
 Copyright (C) 2001-2003 Paul Davis
 Copyright (C) 2005-2012 Grame
 Copyright (C) 2013 Samsung Electronics
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation; either version 2.1 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 */

#ifndef __jack_shm_android_h__
#define __jack_shm_android_h__

#include <limits.h>
#include <sys/types.h>
#include "types.h"
#include "JackCompilerDeps.h"
#include <binder/MemoryHeapBase.h>
#include <utils/RefBase.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define JACK_SHM_REGISTRY_FD        -1
#define JACK_SHM_HEAP_ENOUGH_COUNT  300

void jack_instantiate();

#ifdef __cplusplus
}
#endif

namespace android {

    class IAndroidShm;

    class Shm {
        public:
            static Shm* Instantiate();
            virtual ~Shm();
        private:
            Shm();
            Shm( const Shm&);
            Shm& operator=(const Shm);

        private:
            void set_server_prefix (const char *server_name);
            int server_initialize_shm (int new_registry);
            int shm_lock_registry (void);
            void shm_unlock_registry (void);
            int access_registry (jack_shm_info_t *ri);
            void remove_shm (jack_shm_id_t *id);
            int create_registry (jack_shm_info_t *ri);
            int shm_validate_registry ();
            int GetUID();
            int GetPID();
            void shm_init_registry ();
            void release_shm_entry (jack_shm_registry_index_t index);
            jack_shm_registry_t * get_free_shm_info ();

        public:
            static void jack_shm_copy_from_registry (jack_shm_info_t*, jack_shm_registry_index_t);
            static void jack_shm_copy_to_registry (jack_shm_info_t*, jack_shm_registry_index_t*);
            static int jack_release_shm_info (jack_shm_registry_index_t);
            static char* jack_shm_addr (jack_shm_info_t* si);
            static int jack_register_server (const char *server_name, int new_registry);
            static int jack_unregister_server (const char *server_name);
            static int jack_initialize_shm (const char *server_name);
            static int jack_initialize_shm_server (void);
            static int jack_initialize_shm_client (void);
            static int jack_cleanup_shm (void);
            static int jack_shmalloc (const char *shm_name, jack_shmsize_t size, jack_shm_info_t* result);
            static void jack_release_shm (jack_shm_info_t*);
            static void jack_release_lib_shm (jack_shm_info_t*);
            static void jack_destroy_shm (jack_shm_info_t*);
            static int jack_attach_shm (jack_shm_info_t*);
            static int jack_attach_lib_shm (jack_shm_info_t*);
            static int jack_attach_shm_read (jack_shm_info_t*);
            static int jack_attach_lib_shm_read (jack_shm_info_t*);
            static int jack_resize_shm (jack_shm_info_t*, jack_shmsize_t size);

        public:
            void shm_copy_from_registry (jack_shm_info_t*, jack_shm_registry_index_t);
            void shm_copy_to_registry (jack_shm_info_t*, jack_shm_registry_index_t*);
            int release_shm_info (jack_shm_registry_index_t);
            char* shm_addr (unsigned int fd);
            
            // here begin the API
            int register_server (const char *server_name, int new_registry);
            int unregister_server (const char *server_name);
            
            int initialize_shm (const char *server_name);
            int initialize_shm_server (void);
            int initialize_shm_client (void);
            int cleanup_shm (void);
            
            int shmalloc (const char *shm_name, jack_shmsize_t size, jack_shm_info_t* result);
            void release_shm (jack_shm_info_t*);
            void release_lib_shm (jack_shm_info_t*);
            void destroy_shm (jack_shm_info_t*);
            int attach_shm (jack_shm_info_t*);
            int attach_lib_shm (jack_shm_info_t*);
            int attach_shm_read (jack_shm_info_t*);
            int attach_lib_shm_read (jack_shm_info_t*);
            int resize_shm (jack_shm_info_t*, jack_shmsize_t size);

        private:
            static jack_shmtype_t jack_shmtype;
            static jack_shm_id_t   registry_id;
            static jack_shm_fd_t   registry_fd;
            static jack_shm_info_t registry_info;
            static jack_shm_header_t   *jack_shm_header;
            static jack_shm_registry_t *jack_shm_registry;
            static char jack_shm_server_prefix[JACK_SERVER_NAME_SIZE];
            static int semid;
            static pthread_mutex_t mutex;
            static Shm* ref;
            
            void jack_release_shm_entry (jack_shm_registry_index_t index);
            int jack_shm_lock_registry (void);
            void jack_shm_unlock_registry (void);
            
            //static  sp<IAndroidShm>  mShmService;
            static sp<IMemoryHeap> mShmMemBase[JACK_SHM_HEAP_ENOUGH_COUNT];

        public:
            static sp<IAndroidShm> getShmService();
    };
};

#endif /* __jack_shm_android_h__ */
