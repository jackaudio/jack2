#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#include <jack/jack.h>
#include <jack/metadata.h>
#include <jack/uuid.h>
#include <jack/session.h>

static int subject_is_client = 0;
static int subject_is_port = 0;
static jack_uuid_t uuid;
static char* subject;

static void
show_usage (void)
{
	fprintf (stderr, "\nUsage: jack_property [options] UUID [ key [ value [ type  ] ] ]\n");
	fprintf (stderr, "Set/Display JACK properties (metadata).\n\n");
	fprintf (stderr, "Set options:\n");
	fprintf (stderr, "        -s, --set		Set property \"key\" to \"value\" for \"UUID\" with optional MIME type \"type\"\n");
	fprintf (stderr, "        -d, --delete		Remove/delete property \"key\" for \"UUID\"\n");
	fprintf (stderr, "        -d, --delete UUID	Remove/delete all properties for \"UUID\"\n");
	fprintf (stderr, "        -D, --delete-all	Remove/delete all properties\n");
        fprintf (stderr, "        --client		Interpret UUID as a client name, not a UUID\n");
        fprintf (stderr, "        --port		\tInterpret UUID as a port name, not a UUID\n");
	fprintf (stderr, "\nDisplay options:\n");
	fprintf (stderr, "        -l			Show all properties\n");
	fprintf (stderr, "        -l, --list UUID	\tShow value all properties of UUID\n");
	fprintf (stderr, "        -l, --list UUID key	Show value for key of UUID\n");
	fprintf (stderr, "\nFor more information see http://jackaudio.org/\n");
}

static int
get_subject (jack_client_t* client, char* argv[], int* optind)
{
        if (subject_is_client) {
                char* cstr = argv[(*optind)++];
                char* ustr;

                if ((ustr = jack_get_uuid_for_client_name (client, cstr)) == NULL) {
                        fprintf (stderr, "cannot get UUID for client named %s\n", cstr);
                        return -1;
                }
                
                if (jack_uuid_parse (ustr, &uuid)) {
                        fprintf (stderr, "cannot parse client UUID as UUID\n");
                        return -1;
                }

                subject = cstr;

        } else if (subject_is_port) {

                char* pstr = argv[(*optind)++];
                jack_port_t* port;

                if ((port = jack_port_by_name (client, pstr)) == NULL) {
                        fprintf (stderr, "cannot find port name %s\n", pstr);
                        return -1;
                }
                
                uuid = jack_port_uuid (port);
                subject = pstr;

        } else {
                char* str = argv[(*optind)++];
                
                if (jack_uuid_parse (str, &uuid)) {
                        fprintf (stderr, "cannot parse subject as UUID\n");
                        return -1;
                }

                subject = str;
        }

        return 0;
}

int main (int argc, char* argv[])
{
        jack_client_t* client = NULL;
	jack_options_t options = JackNoStartServer;
        char* key = NULL;
        char* value = NULL;
        char* type = NULL;
        int set = 1;
        int delete = 0;
        int delete_all = 0;
        int list_all = 0;
        int c;
	int option_index;
	extern int optind;
	struct option long_options[] = {
		{ "set", 0, 0, 's' },
		{ "delete", 0, 0, 'd' },
		{ "delete-all", 0, 0, 'D' },
		{ "list", 0, 0, 'l' },
		{ "all", 0, 0, 'a' },
                { "client", 0, 0, 'c' },
                { "port", 0, 0, 'p' },
		{ 0, 0, 0, 0 }
	};

        if (argc < 2) {
                show_usage ();
                exit (1);
        }

	while ((c = getopt_long (argc, argv, "sdDlaApc", long_options, &option_index)) >= 0) {
		switch (c) {
		case 's':
                        if (argc < 5) {
                                show_usage ();
                                exit (1);
                        }
			set = 1;
			break;
                case 'd':
                        if (argc < 3) {
                                show_usage ();
                                return 1;
                        }
                        set = 0;
                        delete = 1;
                        break;

                case 'D':
                        delete = 0;
                        set = 0;
                        delete_all = 1;
                        break;

                case 'l':
                        set = 0;
                        delete = 0;
                        delete_all = 0;
                        break;

                case 'a':
                        list_all = 1;
                        set = 0;
                        delete = 0;
                        delete_all = 0;
                        break;

                case 'p':
                        subject_is_port = 1;
                        break;

                case 'c':
                        subject_is_client = 1;
                        break;

                case '?':
                default:
                        show_usage ();
                        exit (1);
                }
        }

        if ((client = jack_client_open ("jack-property", options, NULL)) == 0) {
                fprintf (stderr, "Cannot connect to JACK server\n");
                exit (1);
        }

        if (delete_all) {

                if (jack_remove_all_properties (client) == 0) {
                        printf ("JACK metadata successfully delete\n");
                        exit (0);
                }
                exit (1);
        }

        if (delete) {

                int args_left = argc - optind;

                if (args_left < 1) {
                        show_usage ();
                        exit (1);
                }

                /* argc == 3: delete all properties for a subject
                   argc == 4: delete value of key for subject
                */

                if (args_left >= 2) {
                        
                        if (get_subject (client, argv, &optind)) {
                                return 1;
                        }

                        key = argv[optind++];

                        if (jack_remove_property (client, uuid, key)) {
                                fprintf (stderr, "\"%s\" property not removed for %s\n", key, subject);
                                exit (1);
                        }

                } else {
                        
                        if (get_subject (client, argv, &optind)) {
                                return 1;
                        }
                        
                        if (jack_remove_properties (client, uuid) < 0) {
                                fprintf (stderr, "cannot remove properties for UUID %s\n", subject);
                                exit (1);
                        }
                }

        }  else if (set) {

                int args_left = argc - optind;

                if (get_subject (client, argv, &optind)) {
                        return -1;
                }

                key = argv[optind++];
                value = argv[optind++];

                if (args_left >= 3) {
                        type = argv[optind++];
                } else {
                        type = "";
                }

                if (jack_set_property (client, uuid, key, value, type)) {
                        fprintf (stderr, "cannot set value for key %s of %s\n", value, subject);
                        exit (1);
                }
                
        } else {

                /* list properties */
                
                int args_left = argc - optind;

                if (args_left >= 2) {

                        /* list properties for a UUID/key pair */

                        if (get_subject (client, argv, &optind)) {
                                return -1;
                        }

                        key = argv[optind++];

                        if (jack_get_property (uuid, key, &value, &type) == 0) {
                                printf ("%s\n", value);
                                free (value);
                                if (type) {
                                        free (type);
                                }
                        } else {
                                fprintf (stderr, "Value not found for %s of %s\n", key, subject);
                                exit (1);
                        }

                } else if (args_left == 1) {

                        /* list all properties for a given UUID */

                        jack_description_t description;
                        size_t cnt, n;

                        if (get_subject (client, argv, &optind)) {
                                return -1;
                        }
                        
                        if ((cnt = jack_get_properties (uuid, &description)) < 0) {
                                fprintf (stderr, "could not retrieve properties for %s\n", subject);
                                exit (1);
                        }

                        for (n = 0; n < cnt; ++n) {
                                if (description.properties[n].type) {
                                        printf ("key: %s value: %s type: %s\n", 
                                                description.properties[n].key, 
                                                description.properties[n].data,
                                                description.properties[n].type);
                                } else {
                                        printf ("key: %s value: %s\n", 
                                                description.properties[n].key, 
                                                description.properties[n].data);
                                }
                        }

                        jack_free_description (&description, 0);

                } else {

                        /* list all properties */

                        jack_description_t* description;
                        size_t cnt;
                        size_t p;
                        size_t n;
                        char buf[JACK_UUID_STRING_SIZE];

                        if ((cnt = jack_get_all_properties (&description)) < 0) {
                                fprintf (stderr, "could not retrieve properties for %s\n", subject);
                                exit (1);
                        }

                        for (n = 0; n < cnt; ++n) {
                                jack_uuid_unparse (description[n].subject, buf);
                                printf ("%s\n", buf);
                                for (p = 0; p < description[n].property_cnt; ++p) {
                                        if (description[n].properties[p].type) {
                                                printf ("key: %s value: %s type: %s\n", 
                                                        description[n].properties[p].key, 
                                                        description[n].properties[p].data,
                                                        description[n].properties[p].type);
                                        } else {
                                                printf ("key: %s value: %s\n", 
                                                        description[n].properties[p].key, 
                                                        description[n].properties[p].data);
                                        }
                                }
                                jack_free_description (&description[n], 0);
                        }

                        free (description);
                }
        }


        (void) jack_client_close (client);
        return 0;
}
