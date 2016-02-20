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

#define LOG_TAG "JAMSHMSERVICE"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <binder/MemoryHeapBase.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <utils/Log.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include "BnAndroidShm.h"
#include "AndroidShm.h"

#include "JackConstants.h"

#include <fcntl.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>
#include <dirent.h>
#include <sys/mman.h>
#include <linux/ashmem.h>
#include <cutils/ashmem.h>

#include "JackError.h"

// remove ALOGI log
#define jack_d 
//#define jack_d ALOGI
#define jack_error ALOGE
#define MEMORY_SIZE 10*1024


namespace android {

   jack_shmtype_t Shm::jack_shmtype = shm_ANDROID;

   /* The JACK SHM registry is a chunk of memory for keeping track of the
     * shared memory used by each active JACK server.  This allows the
     * server to clean up shared memory when it exits.  To avoid memory
     * leakage due to kill -9, crashes or debugger-driven exits, this
     * cleanup is also done when a new instance of that server starts.
     */
    
    /* per-process global data for the SHM interfaces */
    jack_shm_id_t   Shm::registry_id; /* SHM id for the registry */
    jack_shm_fd_t   Shm::registry_fd = JACK_SHM_REGISTRY_FD;
    
    jack_shm_info_t Shm::registry_info = {
      JACK_SHM_NULL_INDEX, 0, 0, { MAP_FAILED }
    };
    
    /* pointers to registry header and array */
    jack_shm_header_t   *Shm::jack_shm_header = NULL;
    jack_shm_registry_t *Shm::jack_shm_registry = NULL;
    char Shm::jack_shm_server_prefix[JACK_SERVER_NAME_SIZE+1] = "";
    
    /* jack_shm_lock_registry() serializes updates to the shared memory
     * segment JACK uses to keep track of the SHM segments allocated to
     * all its processes, including multiple servers.
     *
     * This is not a high-contention lock, but it does need to work across
     * multiple processes. High transaction rates and realtime safety are
     * not required. Any solution needs to at least be portable to POSIX
     * and POSIX-like systems.
     *
     * We must be particularly careful to ensure that the lock be released
     * if the owning process terminates abnormally. Otherwise, a segfault
     * or kill -9 at the wrong moment could prevent JACK from ever running
     * again on that machine until after a reboot.
     */
    
    #define JACK_SEMAPHORE_KEY 0x282929
    #define JACK_SHM_REGISTRY_KEY JACK_SEMAPHORE_KEY
    #define JACK_REGISTRY_NAME "/jack-shm-registry"

    int Shm::semid = -1;
    pthread_mutex_t Shm::mutex = PTHREAD_MUTEX_INITIALIZER;

    //sp<IAndroidShm> Shm::mShmService;
    sp<IMemoryHeap> Shm::mShmMemBase[JACK_SHM_HEAP_ENOUGH_COUNT] = {0,};

    Shm* Shm::ref = NULL;

    Shm* Shm::Instantiate() {
        if(Shm::ref == NULL) {
            jack_d("shm::Instantiate is called");
            Shm::ref = new Shm;
            //AndroidShm::instantiate();
        }
        return ref;
    }

    Shm::Shm() { }

    Shm::~Shm() { }

	sp<IAndroidShm> Shm::getShmService(){
		return interface_cast<IAndroidShm>(defaultServiceManager()->getService(String16("com.samsung.android.jam.IAndroidShm")));
	}

    //sp<IAndroidShm>& Shm::getShmService() {
    //    if (mShmService.get() == 0) {
    //        sp<IServiceManager> sm = defaultServiceManager();
    //        sp<IBinder> binder;
    //        do {
    //            binder = sm->getService(String16("com.samsung.android.jam.IAndroidShm"));
    //            if (binder != 0)
    //                break;
    //            ALOGW("CameraService not published, waiting...");
    //            usleep(500000); // 0.5 s
    //        } while(true);
    //        mShmService = interface_cast<IAndroidShm>(binder);
    //    }
    //    ALOGE_IF(mShmService==0, "no CameraService!?");
    //    return mShmService;
    //}

    void Shm::shm_copy_from_registry (jack_shm_info_t* /*si*/, jack_shm_registry_index_t ) {
        // not used
    }
    void Shm::shm_copy_to_registry (jack_shm_info_t* /*si*/, jack_shm_registry_index_t*) {
        // not used
    }

    void Shm::jack_release_shm_entry (jack_shm_registry_index_t index) {
        /* the registry must be locked */
        jack_shm_registry[index].size = 0;
        jack_shm_registry[index].allocator = 0;
        memset (&jack_shm_registry[index].id, 0,
            sizeof (jack_shm_registry[index].id));
        jack_shm_registry[index].fd = 0;
    }

    int Shm::release_shm_info (jack_shm_registry_index_t index) {
        /* must NOT have the registry locked */
        if (jack_shm_registry[index].allocator == GetPID()) {
            if (jack_shm_lock_registry () < 0) {
                jack_error ("jack_shm_lock_registry fails...");
                return -1;
            }
            jack_release_shm_entry (index);
            jack_shm_unlock_registry ();
            jack_d ("release_shm_info: success!");
        }
        else
            jack_error ("release_shm_info: error!");
        
        return 0;
    }
    char* Shm::shm_addr (unsigned int fd) {
        if(fd >= JACK_SHM_HEAP_ENOUGH_COUNT) {
            jack_error("ignore to get memory buffer : index[%d] is too big", fd);
            return NULL;
        }

		sp<IAndroidShm> service = Shm::getShmService();
		if(service == NULL){
			jack_error("shm service is null");
			return NULL;
		}
        mShmMemBase[fd] = service->getBuffer(fd);
        if(mShmMemBase[fd] == NULL) {
            jack_error("fail to get memory buffer");
            return NULL;
        }

        return ((char *) mShmMemBase[fd]->getBase());
    }

    int Shm::shm_lock_registry (void) {
        pthread_mutex_lock (&mutex);
        return 0;
    }

    void Shm::shm_unlock_registry (void) {
        pthread_mutex_unlock (&mutex);
    }
    
    void Shm::release_shm_entry (jack_shm_registry_index_t index) {
        /* the registry must be locked */
        jack_shm_registry[index].size = 0;
        jack_shm_registry[index].allocator = 0;
        memset (&jack_shm_registry[index].id, 0,
        sizeof (jack_shm_registry[index].id));
    }

    void Shm::remove_shm (jack_shm_id_t *id) {
        int shm_fd = -1;
        jack_d("remove_id [%s]",(char*)id);
        if(!strcmp((const char*)id, JACK_REGISTRY_NAME)) {
            shm_fd = registry_fd;
        } else {
            for (int i = 0; i < MAX_SHM_ID; i++) {
                if(!strcmp((const char*)id, jack_shm_registry[i].id)) {
                    shm_fd = jack_shm_registry[i].fd;
                    break;
                }
            }
        }

        if (shm_fd >= 0) {
			sp<IAndroidShm> service = getShmService();
            if(service != NULL) {
                service->removeShm(shm_fd);
            } else {
            	jack_error("shm service is null");
            }
        }
        jack_d ("[APA] jack_remove_shm : ok ");
    }

    int Shm::access_registry (jack_shm_info_t * ri) {
        jack_d("access_registry\n");
        /* registry must be locked */
		sp<IAndroidShm> service = getShmService();
		if(service == NULL){
			jack_error("shm service is null");
			return EINVAL;
		}
        int shm_fd = service->getRegistryIndex();

        strncpy (registry_id, JACK_REGISTRY_NAME, sizeof (registry_id) - 1);
        registry_id[sizeof (registry_id) - 1] = '\0';

        if(service->isAllocated(shm_fd) == FALSE) {
            //jack_error ("Cannot mmap shm registry segment (%s)",
            //        strerror (errno));
            jack_error ("Cannot mmap shm registry segment");
            //close (shm_fd);
            ri->ptr.attached_at = NULL;
            registry_fd = JACK_SHM_REGISTRY_FD;
            return EINVAL;
        }

        ri->fd = shm_fd;
        registry_fd = shm_fd;
        ri->ptr.attached_at = shm_addr(shm_fd);

        if(ri->ptr.attached_at == NULL) {
            ALOGE("attached pointer is null !");
            jack_shm_header = NULL;
            jack_shm_registry = NULL;
            return 0;
        }

        /* set up global pointers */
        ri->index = JACK_SHM_REGISTRY_INDEX;
        jack_shm_header = (jack_shm_header_t*)(ri->ptr.attached_at);
        jack_shm_registry = (jack_shm_registry_t *) (jack_shm_header + 1);

        jack_d("jack_shm_header[%p],jack_shm_registry[%p]", jack_shm_header, jack_shm_registry);
        //close (shm_fd); // steph
        return 0;
    }

    int Shm::GetUID() {
        return getuid();
    }

    int Shm::GetPID() {
        return getpid();
    }

    int Shm::jack_shm_lock_registry (void) {
        // TODO: replace semaphore to mutex
        pthread_mutex_lock (&mutex);
        return 0;
    }
    
    void Shm::jack_shm_unlock_registry (void) {
        // TODO: replace semaphore to mutex
        pthread_mutex_unlock (&mutex);
        return;
    }

    void Shm::shm_init_registry () {
        if(jack_shm_header == NULL)
            return;

        /* registry must be locked */

        memset (jack_shm_header, 0, JACK_SHM_REGISTRY_SIZE);

        jack_shm_header->magic = JACK_SHM_MAGIC;
        //jack_shm_header->protocol = JACK_PROTOCOL_VERSION;
        jack_shm_header->type = jack_shmtype;
        jack_shm_header->size = JACK_SHM_REGISTRY_SIZE;
        jack_shm_header->hdr_len = sizeof (jack_shm_header_t);
        jack_shm_header->entry_len = sizeof (jack_shm_registry_t);

        for (int i = 0; i < MAX_SHM_ID; ++i) {
            jack_shm_registry[i].index = i;
        }
    }

    void Shm::set_server_prefix (const char *server_name) {
        snprintf (jack_shm_server_prefix, sizeof (jack_shm_server_prefix),
              "jack-%d:%s:", GetUID(), server_name);
    }

    /* create a new SHM registry segment
     *
     * sets up global registry pointers, if successful
     *
     * returns: 0 if registry created successfully
     *          nonzero error code if unable to allocate a new registry
     */
    int Shm::create_registry (jack_shm_info_t * ri) {
        jack_d("create_registry\n");
        /* registry must be locked */
        int shm_fd = 0;

        strncpy (registry_id, JACK_REGISTRY_NAME, sizeof (registry_id) - 1);
        registry_id[sizeof (registry_id) - 1] = '\0';

		sp<IAndroidShm> service = getShmService();
		if(service == NULL){
			jack_error("shm service is null");
			return EINVAL;
		}

        if((shm_fd = service->allocShm(JACK_SHM_REGISTRY_SIZE)) < 0) {
            jack_error("Cannot create shm registry segment");
            registry_fd = JACK_SHM_REGISTRY_FD;
            return EINVAL;
        }

        service->setRegistryIndex(shm_fd);

        /* set up global pointers */
        ri->fd = shm_fd;
        ri->index = JACK_SHM_REGISTRY_INDEX;
        registry_fd = shm_fd;
        ri->ptr.attached_at = shm_addr(shm_fd);
        ri->size = JACK_SHM_REGISTRY_SIZE;

        jack_shm_header = (jack_shm_header_t*)(ri->ptr.attached_at);
        jack_shm_registry = (jack_shm_registry_t *) (jack_shm_header + 1);

        jack_d("create_registry jack_shm_header[%p], jack_shm_registry[%p]", jack_shm_header, jack_shm_registry);

        /* initialize registry contents */
        shm_init_registry ();
        //close (shm_fd); // steph

        return 0;
    }
    
    int Shm::shm_validate_registry () {
        /* registry must be locked */
        if(jack_shm_header == NULL) {
            return -1;
        }
    
        if ((jack_shm_header->magic == JACK_SHM_MAGIC)
            //&& (jack_shm_header->protocol == JACK_PROTOCOL_VERSION)
            && (jack_shm_header->type == jack_shmtype)
            && (jack_shm_header->size == JACK_SHM_REGISTRY_SIZE)
            && (jack_shm_header->hdr_len == sizeof (jack_shm_header_t))
            && (jack_shm_header->entry_len == sizeof (jack_shm_registry_t))) {
    
            return 0;       /* registry OK */
        }
    
        return -1;
    }

    int Shm::server_initialize_shm (int new_registry) {
        int rc;
    
        jack_d("server_initialize_shm\n");
    
        if (jack_shm_header)
            return 0;        /* already initialized */
    
        if (shm_lock_registry () < 0) {
            jack_error ("jack_shm_lock_registry fails...");
            return -1;
        }
    
        rc = access_registry (&registry_info);
    
        if (new_registry) {
            remove_shm (&registry_id);
            rc = ENOENT;
        }
    
        switch (rc) {
        case ENOENT:        /* registry does not exist */
            rc = create_registry (&registry_info);
            break;
        case 0:                /* existing registry */
            if (shm_validate_registry () == 0)
                break;
            /* else it was invalid, so fall through */
        case EINVAL:            /* bad registry */
            /* Apparently, this registry was created by an older
             * JACK version.  Delete it so we can try again. */
            release_shm (&registry_info);
            remove_shm (&registry_id);
            if ((rc = create_registry (&registry_info)) != 0) {
                //jack_error ("incompatible shm registry (%s)",
                //        strerror (errno));
                jack_error ("incompatible shm registry");
//#ifndef USE_POSIX_SHM
//            jack_error ("to delete, use `ipcrm -M 0x%0.8x'", JACK_SHM_REGISTRY_KEY);
//#endif
            }
            break;
        default:            /* failure return code */
            break;
        }
    
        shm_unlock_registry ();
        return rc;
    }

    // here begin the API
    int Shm::register_server (const char *server_name, int new_registry) {
        int i, res = 0;

        jack_d("register_server new_registry[%d]\n", new_registry);

        set_server_prefix (server_name);

        if (server_initialize_shm (new_registry))
           return ENOMEM;

        if (shm_lock_registry () < 0) {
           jack_error ("jack_shm_lock_registry fails...");
           return -1;
        }

        /* See if server_name already registered.  Since server names
         * are per-user, we register the unique server prefix string.
         */
        for (i = 0; i < MAX_SERVERS; i++) {

           if (strncmp (jack_shm_header->server[i].name,
                    jack_shm_server_prefix,
                    JACK_SERVER_NAME_SIZE) != 0)
               continue;   /* no match */

           if (jack_shm_header->server[i].pid == GetPID()) {
               res = 0; /* it's me */
               goto unlock;
           }

           /* see if server still exists */
           if (kill (jack_shm_header->server[i].pid, 0) == 0)  {
               res = EEXIST;   /* other server running */
               goto unlock;
           }

           /* it's gone, reclaim this entry */
           memset (&jack_shm_header->server[i], 0,
               sizeof (jack_shm_server_t));
        }

        /* find a free entry */
        for (i = 0; i < MAX_SERVERS; i++) {
           if (jack_shm_header->server[i].pid == 0)
               break;
        }

        if (i >= MAX_SERVERS) {
           res = ENOSPC;       /* out of space */
           goto unlock;
        }

        /* claim it */
        jack_shm_header->server[i].pid = GetPID();
        strncpy (jack_shm_header->server[i].name,
            jack_shm_server_prefix,
            JACK_SERVER_NAME_SIZE - 1);
        jack_shm_header->server[i].name[JACK_SERVER_NAME_SIZE - 1] = '\0';

        unlock:
        shm_unlock_registry ();
        return res;
    }
    
    int Shm::unregister_server (const char * /* server_name */) {
        int i;
        if (shm_lock_registry () < 0) {
            jack_error ("jack_shm_lock_registry fails...");
            return -1;
        }
        
        for (i = 0; i < MAX_SERVERS; i++) {
            if (jack_shm_header->server[i].pid == GetPID()) {
                memset (&jack_shm_header->server[i], 0,
                    sizeof (jack_shm_server_t));
            }
        }
        
        shm_unlock_registry ();
        return 0;
    }

    int Shm::initialize_shm (const char *server_name) {
        int rc;
        
        if (jack_shm_header)
            return 0;       /* already initialized */
        
        set_server_prefix (server_name);
        
        if (shm_lock_registry () < 0) {
            jack_error ("jack_shm_lock_registry fails...");
            return -1;
        }
        
        if ((rc = access_registry (&registry_info)) == 0) {
            if ((rc = shm_validate_registry ()) != 0) {
                jack_error ("Incompatible shm registry, "
                        "are jackd and libjack in sync?");
            }
        }
        shm_unlock_registry ();
        
        return rc;
    }
    
    int Shm::initialize_shm_server (void) {
        // not used
        return 0;
    }

    int Shm::initialize_shm_client (void) {
        // not used
        return 0;
    }

    int Shm::cleanup_shm (void) {
        int i;
        int destroy;
        jack_shm_info_t copy;
        
        if (shm_lock_registry () < 0) {
            jack_error ("jack_shm_lock_registry fails...");
            return -1;
        }
        
        for (i = 0; i < MAX_SHM_ID; i++) {
            jack_shm_registry_t* r;
        
            r = &jack_shm_registry[i];
            memcpy (&copy, r, sizeof (jack_shm_info_t));
            destroy = FALSE;
        
            /* ignore unused entries */
            if (r->allocator == 0)
                continue;
        
            /* is this my shm segment? */
            if (r->allocator == GetPID()) {
        
                /* allocated by this process, so unattach
                   and destroy. */
                release_shm (&copy);
                destroy = TRUE;
        
            } else {
        
                /* see if allocator still exists */
                if (kill (r->allocator, 0)) {
                    if (errno == ESRCH) {
                        /* allocator no longer exists,
                         * so destroy */
                        destroy = TRUE;
                    }
                }
            }
        
            if (destroy) {
        
                int index = copy.index;
        
                if ((index >= 0)  && (index < MAX_SHM_ID)) {
                    remove_shm (&jack_shm_registry[index].id);
                    release_shm_entry (index);
                }
                r->size = 0;
                r->allocator = 0;
            }
        }
        
        shm_unlock_registry ();
        return TRUE;

    }

    jack_shm_registry_t * Shm::get_free_shm_info () {
        /* registry must be locked */
        jack_shm_registry_t* si = NULL;
        int i;

        for (i = 0; i < MAX_SHM_ID; ++i) {
            if (jack_shm_registry[i].size == 0) {
                break;
            }
        }

        if (i < MAX_SHM_ID) {
            si = &jack_shm_registry[i];
        }

        return si;
    }
    
    int Shm::shmalloc (const char * /*shm_name*/, jack_shmsize_t size, jack_shm_info_t* si) {
        jack_shm_registry_t* registry;
        int shm_fd;
        int rc = -1;
        char name[SHM_NAME_MAX+1];
 
        if (shm_lock_registry () < 0) {
           jack_error ("jack_shm_lock_registry fails...");
           return -1;
        }

		sp<IAndroidShm> service = getShmService();
		if(service == NULL){
			rc = errno;
			jack_error("shm service is null");
			goto unlock;
		}
 
        if ((registry = get_free_shm_info ()) == NULL) {
           jack_error ("shm registry full");
           goto unlock;
        }

        snprintf (name, sizeof (name), "/jack-%d-%d", GetUID(), registry->index);
        if (strlen (name) >= sizeof (registry->id)) {
           jack_error ("shm segment name too long %s", name);
           goto unlock;
        }

        if((shm_fd = service->allocShm(size)) < 0) {
           rc = errno;
           jack_error ("Cannot create shm segment %s", name);
           goto unlock;
        }
        
        //close (shm_fd);
        registry->size = size;
        strncpy (registry->id, name, sizeof (registry->id) - 1);
        registry->id[sizeof (registry->id) - 1] = '\0';
        registry->allocator = GetPID();
        registry->fd = shm_fd;
        si->fd = shm_fd;
        si->index = registry->index;
        si->ptr.attached_at = MAP_FAILED;  /* not attached */
        rc = 0;  /* success */
        
        jack_d ("[APA] jack_shmalloc : ok ");

unlock:
        shm_unlock_registry ();
        return rc;
    }
    
    void Shm::release_shm (jack_shm_info_t* /*si*/) {
        // do nothing
    }
    
    void Shm::release_lib_shm (jack_shm_info_t* /*si*/) {
        // do nothing
    }
    
    void Shm::destroy_shm (jack_shm_info_t* si) {
        /* must NOT have the registry locked */
        if (si->index == JACK_SHM_NULL_INDEX)
            return;            /* segment not allocated */

        remove_shm (&jack_shm_registry[si->index].id);
        release_shm_info (si->index);
    }
    
    int Shm::attach_shm (jack_shm_info_t* si) {
        jack_shm_registry_t *registry = &jack_shm_registry[si->index];

        if((si->ptr.attached_at = shm_addr(registry->fd)) == NULL) {
            jack_error ("Cannot mmap shm segment %s", registry->id);
            close (si->fd);
            return -1;
        }
        return 0;
    }
    
    int Shm::attach_lib_shm (jack_shm_info_t* si) {
        int res = attach_shm(si);
        if (res == 0)
            si->size = jack_shm_registry[si->index].size; // Keep size in si struct
        return res;
    }
    
    int Shm::attach_shm_read (jack_shm_info_t* si) {
        jack_shm_registry_t *registry = &jack_shm_registry[si->index];

        if((si->ptr.attached_at = shm_addr(registry->fd)) == NULL) {
            jack_error ("Cannot mmap shm segment %s", registry->id);
            close (si->fd);
            return -1;
        }
        return 0;
    }
    
    int Shm::attach_lib_shm_read (jack_shm_info_t* si) {
        int res = attach_shm_read(si);
        if (res == 0)
            si->size = jack_shm_registry[si->index].size; // Keep size in si struct
        return res;
    }
    
    int Shm::resize_shm (jack_shm_info_t* si, jack_shmsize_t size) {
        jack_shm_id_t id;
        
        /* The underlying type of `id' differs for SYSV and POSIX */
        memcpy (&id, &jack_shm_registry[si->index].id, sizeof (id));
        
        release_shm (si);
        destroy_shm (si);
        
        if (shmalloc ((char *) id, size, si)) {
            return -1;
        }
        return attach_shm (si);
    }

    void Shm::jack_shm_copy_from_registry (jack_shm_info_t* si, jack_shm_registry_index_t t) {
        Shm::Instantiate()->shm_copy_from_registry(si,t);
    }
    void Shm::jack_shm_copy_to_registry (jack_shm_info_t* si, jack_shm_registry_index_t* t) {
        Shm::Instantiate()->shm_copy_to_registry(si,t);
    }
    int Shm::jack_release_shm_info (jack_shm_registry_index_t t) {
        return Shm::Instantiate()->release_shm_info(t);
    }
    char* Shm::jack_shm_addr (jack_shm_info_t* si) {
        if(si != NULL) {
            return (char*)si->ptr.attached_at;
        } else {
            jack_error ("jack_shm_addr : jack_shm_info_t is NULL!");
            return NULL;
        }
    }
    int Shm::jack_register_server (const char *server_name, int new_registry) {
        return Shm::Instantiate()->register_server(server_name, new_registry);
    }
    int Shm::jack_unregister_server (const char *server_name) {
        return Shm::Instantiate()->unregister_server(server_name);
    }
    int Shm::jack_initialize_shm (const char *server_name) {
        return Shm::Instantiate()->initialize_shm(server_name);
    }
    int Shm::jack_initialize_shm_server (void) {
        return Shm::Instantiate()->initialize_shm_server();
    }
    int Shm::jack_initialize_shm_client () {
        return Shm::Instantiate()->initialize_shm_client();
    }
    int Shm::jack_cleanup_shm (void) {
        return Shm::Instantiate()->cleanup_shm();
    }
    int Shm::jack_shmalloc (const char *shm_name, jack_shmsize_t size, jack_shm_info_t* result) {
        return Shm::Instantiate()->shmalloc(shm_name, size, result);
    }
    void Shm::jack_release_shm (jack_shm_info_t* si) {
        Shm::Instantiate()->release_shm(si);
    }
    void Shm::jack_release_lib_shm (jack_shm_info_t* si) {
        Shm::Instantiate()->release_lib_shm(si);
    }
    void Shm::jack_destroy_shm (jack_shm_info_t* si) {
        Shm::Instantiate()->destroy_shm(si);
    }
    int Shm::jack_attach_shm (jack_shm_info_t* si) {
        return Shm::Instantiate()->attach_shm(si);
    }
    int Shm::jack_attach_lib_shm (jack_shm_info_t* si) {
        return Shm::Instantiate()->attach_lib_shm(si);
    }
    int Shm::jack_attach_shm_read (jack_shm_info_t* si) {
        return Shm::Instantiate()->attach_shm_read(si);
    }
    int Shm::jack_attach_lib_shm_read (jack_shm_info_t* si) {
        return Shm::Instantiate()->attach_lib_shm_read(si);
    }
    int Shm::jack_resize_shm (jack_shm_info_t* si, jack_shmsize_t size) {
        return Shm::Instantiate()->resize_shm(si, size);
    }
};

void jack_shm_copy_from_registry (jack_shm_info_t* si, jack_shm_registry_index_t t) { 
    android::Shm::jack_shm_copy_from_registry(si, t); 
}
void jack_shm_copy_to_registry (jack_shm_info_t* si, jack_shm_registry_index_t* t) { 
    android::Shm::jack_shm_copy_to_registry(si, t); 
}
int jack_release_shm_info (jack_shm_registry_index_t t) { 
    return android::Shm::jack_release_shm_info(t); 
}
char* jack_shm_addr (jack_shm_info_t* si) {
    return android::Shm::jack_shm_addr(si); 
}
int jack_register_server (const char *server_name, int new_registry) { 
    return android::Shm::jack_register_server(server_name, new_registry); 
}
int jack_unregister_server (const char *server_name) { 
    return android::Shm::jack_unregister_server(server_name); 
}
int jack_initialize_shm (const char *server_name) { 
    return android::Shm::jack_initialize_shm(server_name); 
}
int jack_initialize_shm_server (void) { 
    return android::Shm::jack_initialize_shm_server(); 
}
int jack_initialize_shm_client (void) { 
    return android::Shm::jack_initialize_shm_client(); 
}
int jack_cleanup_shm (void) { 
    return android::Shm::jack_cleanup_shm(); 
}
int jack_shmalloc (const char *shm_name, jack_shmsize_t size, jack_shm_info_t* result) { 
    return android::Shm::jack_shmalloc(shm_name, size, result); 
}
void jack_release_shm (jack_shm_info_t* si) { 
    android::Shm::jack_release_shm(si); 
}
void jack_release_lib_shm (jack_shm_info_t* si) { 
    android::Shm::jack_release_lib_shm(si); 
}
void jack_destroy_shm (jack_shm_info_t* si) { 
    android::Shm::jack_destroy_shm(si); 
}
int jack_attach_shm (jack_shm_info_t* si) { 
    return android::Shm::jack_attach_shm(si); 
}
int jack_attach_lib_shm (jack_shm_info_t* si) { 
    return android::Shm::jack_attach_lib_shm(si); 
}
int jack_attach_shm_read (jack_shm_info_t* si) { 
    return android::Shm::jack_attach_shm_read(si); 
}
int jack_attach_lib_shm_read (jack_shm_info_t* si) { 
    return android::Shm::jack_attach_lib_shm_read(si); 
}
int jack_resize_shm (jack_shm_info_t* si, jack_shmsize_t size) { 
    return android::Shm::jack_resize_shm(si, size); 
}
void jack_instantiate() {
    android::AndroidShm::instantiate();
}

