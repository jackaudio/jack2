/*
  Copyright (C) 2011 David Robillard
  Copyright (C) 2013 Paul Davis

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or (at
  your option) any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
  License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "JackMetadata.h"

#include "JackClient.h"
#include "JackRequest.h"

#include <string.h>
#include <limits.h>


namespace Jack
{

JackMetadata::JackMetadata(const char* server_name)
#if HAVE_DB
    : fDB(NULL), fDBenv(NULL)
#endif
{
    PropertyInit(server_name);
}

JackMetadata::~JackMetadata()
{
#if HAVE_DB
    if (fDB) {
        fDB->close (fDB, 0);
        fDB = NULL;
    }
    if (fDBenv) {
        fDBenv->close (fDBenv, 0);
        fDBenv = 0;
    }
#endif
}

int JackMetadata::PropertyInit(const char* server_name)
{
#if HAVE_DB

    int ret;
    char dbpath[PATH_MAX + 1];

    /* idempotent */

    if (fDBenv) {
        return 0;
    }

    if ((ret = db_env_create (&fDBenv, 0)) != 0) {
        jack_error ("cannot initialize DB environment: %s\n", db_strerror (ret));
        return -1;
    }

    if ((ret = fDBenv->open (fDBenv, jack_server_dir /*FIXME:(server_name, server_dir)*/, DB_CREATE | DB_INIT_LOCK | DB_INIT_MPOOL | DB_THREAD, 0)) != 0) {
        jack_error ("cannot open DB environment: %s", db_strerror (ret));
        return -1;
    }

    if ((ret = db_create (&fDB, fDBenv, 0)) != 0) {
        jack_error ("Cannot initialize metadata DB (%s)", db_strerror (ret));
        return -1;
    }

    snprintf (dbpath, sizeof(dbpath), "%s/%s", jack_server_dir /*FIXME:(server_name, server_dir)*/, "metadata.db");

    if ((ret = fDB->open (fDB, NULL, dbpath, NULL, DB_HASH, DB_CREATE | DB_THREAD, 0666)) != 0) {
        jack_error ("Cannot open metadata DB at %s: %s", dbpath, db_strerror (ret));
        fDB->close (fDB, 0);
        fDB = NULL;
        return -1;
    }

    return 0;

#else // !HAVE_DB
	return -1;
#endif
}

int JackMetadata::PropertyChangeNotify(JackClient* client, jack_uuid_t subject, const char* key, jack_property_change_t change)
{
    /* the engine passes in a NULL client when it removes metadata during port or client removal
     */

    if (client == NULL) {
        return 0;
    }

    return client->NotifyPropertyChange(subject, key, change);
}

#if HAVE_DB
void JackMetadata::MakeKeyDbt(DBT* dbt, jack_uuid_t subject, const char* key)
{
    char ustr[JACK_UUID_STRING_SIZE];
    size_t len1, len2;

    memset (dbt, 0, sizeof(DBT));
    memset (ustr, 0, JACK_UUID_STRING_SIZE);
    jack_uuid_unparse (subject, ustr);
    len1 = JACK_UUID_STRING_SIZE;
    len2 = strlen (key) + 1;
    dbt->size = len1 + len2;
    dbt->data = malloc (dbt->size);
    memcpy (dbt->data, ustr, len1);         // copy subject+null
    memcpy ((char *)dbt->data + len1, key, len2);   // copy key+null
}
#endif

int JackMetadata::SetProperty(JackClient* client, jack_uuid_t subject, const char* key, const char* value, const char* type)
{
#if HAVE_DB

    DBT d_key;
    DBT data;
    int ret;
    size_t len1, len2;
    jack_property_change_t change;

    if (!key || key[0] == '\0') {
        jack_error ("empty key string for metadata not allowed");
        return -1;
    }

    if (!value || value[0] == '\0') {
        jack_error ("empty value string for metadata not allowed");
        return -1;
    }

    if (PropertyInit(NULL)) {
        return -1;
    }

    /* build a key */

    MakeKeyDbt(&d_key, subject, key);

    /* build data */

    memset (&data, 0, sizeof(data));

    len1 = strlen (value) + 1;
    if (type && type[0] != '\0') {
        len2 = strlen (type) + 1;
    } else {
        len2 = 0;
    }

    data.size = len1 + len2;
    data.data = malloc (data.size);
    memcpy (data.data, value, len1);

    if (len2) {
        memcpy ((char *)data.data + len1, type, len2);
    }

    if (fDB->exists (fDB, NULL, &d_key, 0) == DB_NOTFOUND) {
        change = PropertyCreated;
    } else {
        change = PropertyChanged;
    }

    if ((ret = fDB->put (fDB, NULL, &d_key, &data, 0)) != 0) {
        char ustr[JACK_UUID_STRING_SIZE];
        jack_uuid_unparse (subject, ustr);
        jack_error ("Cannot store metadata for %s/%s (%s)", ustr, key, db_strerror (ret));
        if (d_key.size > 0) {
            free (d_key.data);
        }
        if (data.size  > 0) {
            free (data.data);
        }
        return -1;
    }

    PropertyChangeNotify(client, subject, key, change);

    if (d_key.size > 0) {
        free (d_key.data);
    }
    if (data.size  > 0) {
        free (data.data);
    }

    return 0;

#else // !HAVE_DB
	return -1;
#endif
}

int JackMetadata::GetProperty(jack_uuid_t subject, const char* key, char** value, char** type)
{
#if HAVE_DB

    DBT d_key;
    DBT data;
    int ret;
    size_t len1, len2;

    if (key == NULL || key[0] == '\0') {
        return -1;
    }

    if (PropertyInit(NULL)) {
        return -1;
    }

    /* build a key */

    MakeKeyDbt(&d_key, subject, key);

    /* setup data DBT */

    memset (&data, 0, sizeof(data));
    data.flags = DB_DBT_MALLOC;

    if ((ret = fDB->get (fDB, NULL, &d_key, &data, 0)) != 0) {
        if (ret != DB_NOTFOUND) {
            char ustr[JACK_UUID_STRING_SIZE];
            jack_uuid_unparse (subject, ustr);
            jack_error ("Cannot  metadata for %s/%s (%s)", ustr, key, db_strerror (ret));
        }
        if (d_key.size > 0) {
            free (d_key.data);
        }
        if (data.size  > 0) {
            free (data.data);
        }
        return -1;
    }

    /* result must have at least 2 chars plus 2 nulls to be valid
     */

    if (data.size < 4) {
        if (d_key.size > 0) {
            free (d_key.data);
        }
        if (data.size  > 0) {
            free (data.data);
        }
        return -1;
    }

    len1 = strlen ((const char*)data.data) + 1;
    (*value) = (char*)malloc (len1);
    memcpy (*value, data.data, len1);

    if (len1 < data.size) {
        len2 = strlen ((const char*)data.data + len1) + 1;

        (*type) = (char*)malloc (len2);
        memcpy (*type, (const char *)data.data + len1, len2);
    } else {
        /* no type specified, assume default */
        *type = NULL;
    }

    if (d_key.size > 0) {
        free (d_key.data);
    }
    if (data.size  > 0) {
        free (data.data);
    }

    return 0;

#else // !HAVE_DB
	return -1;
#endif
}

int JackMetadata::GetProperties(jack_uuid_t subject, jack_description_t* desc)
{
#if HAVE_DB

    DBT key;
    DBT data;
    DBC* cursor;
    int ret;
    size_t len1, len2;
    size_t cnt = 0;
    char ustr[JACK_UUID_STRING_SIZE];
    size_t props_size = 0;
    jack_property_t* prop;

    desc->properties = NULL;
    desc->property_cnt = 0;

    memset (ustr, 0, JACK_UUID_STRING_SIZE);
    jack_uuid_unparse (subject, ustr);

    if (PropertyInit(NULL)) {
        return -1;
    }


    if ((ret = fDB->cursor (fDB, NULL, &cursor, 0)) != 0) {
        jack_error ("Cannot create cursor for metadata search (%s)", db_strerror (ret));
        return -1;
    }

    memset (&key, 0, sizeof(key));
    memset (&data, 0, sizeof(data));
    data.flags = DB_DBT_MALLOC;

    while ((ret = cursor->get (cursor, &key, &data, DB_NEXT)) == 0) {

        /* require 2 extra chars (data+null) for key,
           which is composed of UUID str plus a key name
         */

        if (key.size < JACK_UUID_STRING_SIZE + 2) {
            /* if (key.size  > 0) free(key.data); */
            if (data.size > 0) {
                free (data.data);
            }
            continue;
        }

        if (memcmp (ustr, key.data, JACK_UUID_STRING_SIZE) != 0) {
            /* not relevant */
            /* if (key.size  > 0) free(key.data); */
            if (data.size > 0) {
                free (data.data);
            }
            continue;
        }

        /* result must have at least 2 chars plus 2 nulls to be valid
         */

        if (data.size < 4) {
            /* if (key.size  > 0) free(key.data); */
            if (data.size > 0) {
                free (data.data);
            }
            continue;
        }

        /* realloc array if necessary */

        if (cnt == props_size) {
            if (props_size == 0) {
                props_size = 8; /* a rough guess at a likely upper bound for the number of properties */
            } else {
                props_size *= 2;
            }

            desc->properties = (jack_property_t*)realloc (desc->properties, sizeof(jack_property_t) * props_size);
        }

        prop = &desc->properties[cnt];

        /* store UUID/subject */

        jack_uuid_copy (&desc->subject, subject);

        /* copy key (without leading UUID as subject */

        len1 = key.size - JACK_UUID_STRING_SIZE;
        prop->key = (char*)malloc (len1);
        memcpy ((char*)prop->key, (const char *)key.data + JACK_UUID_STRING_SIZE, len1);

        /* copy data (which contains 1 or 2 null terminated strings, the value
           and optionally a MIME type.
         */

        len1 = strlen ((const char*)data.data) + 1;
        prop->data = (char*)malloc (len1);
        memcpy ((char*)prop->data, data.data, len1);

        if (len1 < data.size) {
            len2 = strlen ((const char *)data.data + len1) + 1;

            prop->type = (char*)malloc (len2);
            memcpy ((char*)prop->type, (const char *)data.data + len1, len2);
        } else {
            /* no type specified, assume default */
            prop->type = NULL;
        }

        /* if (key.size  > 0) free(key.data);  */
        if (data.size > 0) {
            free (data.data);
        }

        ++cnt;
    }

    cursor->close (cursor);
    desc->property_cnt = cnt;

    return cnt;

#else // !HAVE_DB
	return -1;
#endif
}

int JackMetadata::GetAllProperties(jack_description_t** descriptions)
{
#if HAVE_DB

    DBT key;
    DBT data;
    DBC* cursor;
    int ret;
    size_t dcnt = 0;
    size_t dsize = 0;
    size_t n = 0;
    jack_description_t* desc = NULL;
    jack_uuid_t uuid = JACK_UUID_EMPTY_INITIALIZER;
    jack_description_t* current_desc = NULL;
    jack_property_t* current_prop = NULL;
    size_t len1, len2;

    if (PropertyInit(NULL)) {
        return -1;
    }

    if ((ret = fDB->cursor (fDB, NULL, &cursor, 0)) != 0) {
        jack_error ("Cannot create cursor for metadata search (%s)", db_strerror (ret));
        return -1;
    }

    memset (&key, 0, sizeof(key));
    memset (&data, 0, sizeof(data));
    data.flags = DB_DBT_MALLOC;

    dsize = 8; /* initial guess at number of descriptions we need */
    dcnt = 0;
    desc = (jack_description_t*)malloc (dsize * sizeof(jack_description_t));

    while ((ret = cursor->get (cursor, &key, &data, DB_NEXT)) == 0) {

        /* require 2 extra chars (data+null) for key,
           which is composed of UUID str plus a key name
         */

        if (key.size < JACK_UUID_STRING_SIZE + 2) {
            /* if (key.size  > 0) free(key.data);  */
            if (data.size > 0) {
                free (data.data);
            }
            continue;
        }

        if (jack_uuid_parse ((const char *)key.data, &uuid) != 0) {
            continue;
        }

        /* do we have an existing description for this UUID */

        for (n = 0; n < dcnt; ++n) {
            if (jack_uuid_compare (uuid, desc[n].subject) == 0) {
                break;
            }
        }

        if (n == dcnt) {
            /* we do not have an existing description, so grow the array */

            if (dcnt == dsize) {
                dsize *= 2;
                desc = (jack_description_t*)realloc (desc, sizeof(jack_description_t) * dsize);
            }

            /* initialize */

            desc[n].property_size = 0;
            desc[n].property_cnt = 0;
            desc[n].properties = NULL;

            /* set up UUID */

            jack_uuid_copy (&desc[n].subject, uuid);
            dcnt++;
        }

        current_desc = &desc[n];

        /* see if there is room for the new property or if we need to realloc
         */

        if (current_desc->property_cnt == current_desc->property_size) {
            if (current_desc->property_size == 0) {
                current_desc->property_size = 8;
            } else {
                current_desc->property_size *= 2;
            }

            current_desc->properties = (jack_property_t*)realloc (current_desc->properties, sizeof(jack_property_t) * current_desc->property_size);
        }

        current_prop = &current_desc->properties[current_desc->property_cnt++];

        /* copy key (without leading UUID) */

        len1 = key.size - JACK_UUID_STRING_SIZE;
        current_prop->key = (char*)malloc (len1);
        memcpy ((char*)current_prop->key, (const char *)key.data + JACK_UUID_STRING_SIZE, len1);

        /* copy data (which contains 1 or 2 null terminated strings, the value
           and optionally a MIME type.
         */

        len1 = strlen ((const char *)data.data) + 1;
        current_prop->data = (char*)malloc (len1);
        memcpy ((char*)current_prop->data, data.data, len1);

        if (len1 < data.size) {
            len2 = strlen ((const char *)data.data + len1) + 1;

            current_prop->type = (char*)malloc (len2);
            memcpy ((char*)current_prop->type, (const char *)data.data + len1, len2);
        } else {
            /* no type specified, assume default */
            current_prop->type = NULL;
        }

        /* if (key.size  > 0) free(key.data);  */
        if (data.size > 0) {
            free (data.data);
        }
    }

    cursor->close (cursor);

    (*descriptions) = desc;

    return dcnt;

#else // !HAVE_DB
	return -1;
#endif
}

int JackMetadata::GetDescription(jack_uuid_t subject, jack_description_t* desc)
{
    return 0;
}

int JackMetadata::GetAllDescriptions(jack_description_t** descs)
{
    return 0;
}

void JackMetadata::FreeDescription(jack_description_t* desc, int free_actual_description_too)
{
    uint32_t n;

    for (n = 0; n < desc->property_cnt; ++n) {
        free ((char*)desc->properties[n].key);
        free ((char*)desc->properties[n].data);
        if (desc->properties[n].type) {
            free ((char*)desc->properties[n].type);
        }
    }

    free (desc->properties);

    if (free_actual_description_too) {
        free (desc);
    }
}

int JackMetadata::RemoveProperty(JackClient* client, jack_uuid_t subject, const char* key)
{
#if HAVE_DB

    DBT d_key;
    int ret;

    if (PropertyInit(NULL)) {
        return -1;
    }

    MakeKeyDbt(&d_key, subject, key);
    if ((ret = fDB->del (fDB, NULL, &d_key, 0)) != 0) {
        jack_error ("Cannot delete key %s (%s)", key, db_strerror (ret));
        if (d_key.size > 0) {
            free (d_key.data);
        }
        return -1;
    }

    PropertyChangeNotify(client, subject, key, PropertyDeleted);

    if (d_key.size > 0) {
        free (d_key.data);
    }

    return 0;

#else // !HAVE_DB
	return -1;
#endif
}

int JackMetadata::RemoveProperties(JackClient* client, jack_uuid_t subject)
{
#if HAVE_DB

    DBT key;
    DBT data;
    DBC* cursor;
    int ret;
    char ustr[JACK_UUID_STRING_SIZE];
    int retval = 0;
    uint32_t cnt = 0;

    memset (ustr, 0, JACK_UUID_STRING_SIZE);
    jack_uuid_unparse (subject, ustr);

    if (PropertyInit(NULL)) {
        return -1;
    }

    if ((ret = fDB->cursor (fDB, NULL, &cursor, 0)) != 0) {
        jack_error ("Cannot create cursor for metadata search (%s)", db_strerror (ret));
        return -1;
    }

    memset (&key, 0, sizeof(key));
    memset (&data, 0, sizeof(data));
    data.flags = DB_DBT_MALLOC;

    while ((ret = cursor->get (cursor, &key, &data, DB_NEXT)) == 0) {

        /* require 2 extra chars (data+null) for key,
           which is composed of UUID str plus a key name
         */

        if (key.size < JACK_UUID_STRING_SIZE + 2) {
            /* if (key.size  > 0) free(key.data); */
            if (data.size > 0) {
                free (data.data);
            }
            continue;
        }

        if (memcmp (ustr, key.data, JACK_UUID_STRING_SIZE) != 0) {
            /* not relevant */
            /* if (key.size  > 0) free(key.data); */
            if (data.size > 0) {
                free (data.data);
            }
            continue;
        }

        if ((ret = cursor->del (cursor, 0)) != 0) {
            jack_error ("cannot delete property (%s)", db_strerror (ret));
            /* don't return -1 here since this would leave things
               even more inconsistent. wait till the cursor is finished
             */
            retval = -1;
        }
        cnt++;

        /* if (key.size  > 0) free(key.data);  */
        if (data.size > 0) {
            free (data.data);
        }
    }

    cursor->close (cursor);

    if (cnt) {
        PropertyChangeNotify(client, subject, NULL, PropertyDeleted);
    }

    if (retval) {
        return -1;
    }

    return cnt;

#else // !HAVE_DB
	return -1;
#endif
}

int JackMetadata::RemoveAllProperties(JackClient* client)
{
#if HAVE_DB

    int ret;
    jack_uuid_t empty_uuid = JACK_UUID_EMPTY_INITIALIZER;

    if (PropertyInit(NULL)) {
        return -1;
    }

    if ((ret = fDB->truncate (fDB, NULL, NULL, 0)) != 0) {
        jack_error ("Cannot clear properties (%s)", db_strerror (ret));
        return -1;
    }

    PropertyChangeNotify(client, empty_uuid, NULL, PropertyDeleted);

    return 0;

#else // !HAVE_DB
	return -1;
#endif
}

} // end of namespace



