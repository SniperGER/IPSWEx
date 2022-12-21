#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <plist/plist.h>
#include <sys/stat.h>

typedef struct _options {
	uint8_t build, version, build_train, product, rootfs, legacy;
} options_t;

static void print_usage(int argc, char *argv[])
{
    char *name = NULL;
    name = strrchr(argv[0], '/');
    printf("Usage: %s <path/to/BuildManifest.plist> [OPTIONS] \n", (name ? name + 1: argv[0]));
    printf("\n");
    printf("Parses information from Restore.plist or BuildManifest.plist\n");
    printf("\n");
    printf("OPTIONS:\n");
    printf("  -b, --build             Prints the build version string of an IPSW\n");
    printf("  -V, --version           Prints the build version of an IPSW\n");
    printf("  -b, --build-train       Prints the build train string of an IPSW\n");
    printf("  -p, --product           Prints the (first, if multiple) supported product identifier\n");
    printf("  -r, --rootfs            Prints the name of the RootFS .dmg\n");
    printf("  -l, --legacy            Sets the \"legacy\" flag for old firmware files containing Restore.plist\n");
	printf("\n");
}

static options_t *parse_arguments(int argc, char *argv[]) {
	options_t *options = (options_t*)calloc(1, sizeof(options_t));

	if (argc <= 2) {
		return NULL;
	}

	for (int i = 2; i < argc; i++) {
		if (!strcmp(argv[i], "--build") || !strcmp(argv[i], "-b")) {
			options->build = 1;
		} else if (!strcmp(argv[i], "--version") || !strcmp(argv[i], "-V")) {
			options->version = 1;
		} else if (!strcmp(argv[i], "--train") || !strcmp(argv[i], "-t")) {
			options->build_train = 1;
		} else if (!strcmp(argv[i], "--product") || !strcmp(argv[i], "-p")) {
			options->product = 1;
		} else if (!strcmp(argv[i], "--rootfs") || !strcmp(argv[i], "-r")) {
			options->rootfs = 1;
		} else if (!strcmp(argv[i], "--legacy") || !strcmp(argv[i], "-l")) {
			options->legacy = 1;
		} else {
			printf("ERROR: Invalid option '%s'\n", argv[i]);
			free(options);
			return NULL;
		}
	}

	return options;
}

int main(int argc, char *argv[]) {
	options_t* options = parse_arguments(argc, argv);

	if (!options) {
        print_usage(argc, argv);
        return 0;
    }

	char* plist = NULL;

	const char* ifile = argv[1];
	FILE* plist_file = fopen(ifile, "rb");

	if (!plist_file) {
		printf("ERROR: Could not open input file '%s': %s\n", ifile, strerror(errno));
		return 1;
	}

	struct stat filestats;
	memset(&filestats, '\0', sizeof(struct stat));
	fstat(fileno(plist_file), &filestats);

	if (filestats.st_size < 8) {
		printf("ERROR: Input file is too small to contain valid plist data.\n");
		fclose(plist_file);
		return -1;
	}

	plist = (char *) malloc(sizeof(char) * (filestats.st_size + 1));
	int read_size = fread(plist, sizeof(char), filestats.st_size, plist_file);

	fclose(plist_file);

	plist_t root_node = NULL;

	if (plist_is_binary(plist, read_size)) {
		plist_from_bin(plist, read_size, &root_node);
	} else {
		plist_from_xml(plist, read_size, &root_node);
	}

	char* str = NULL;

	// printf("%d\n", argc);
	// printf("%u, %u, %u\n", options->build, options->product, options->rootfs);

	if (options->build == 1) {
		plist_t product_build = plist_dict_get_item(root_node, "ProductBuildVersion");
		if (!product_build || plist_get_node_type(product_build) != PLIST_STRING) {
			printf("ERROR: Unable to find product build node\n");
			return -1;
		}

		str = NULL;
		plist_get_string_val(product_build, &str);

		printf("%s\n", str);
		free(str);
	} else if (options->version == 1) {
		plist_t product_version = plist_dict_get_item(root_node, "ProductVersion");
		if (!product_version || plist_get_node_type(product_version) != PLIST_STRING) {
			printf("ERROR: Unable to find product version node\n");
			return -1;
		}

		str = NULL;
		plist_get_string_val(product_version, &str);

		printf("%s\n", str);
		free(str);
	} else if (options->build_train == 1) {
		plist_t build_identities_array = plist_dict_get_item(root_node, "BuildIdentities");
		if (!build_identities_array || plist_get_node_type(build_identities_array) != PLIST_ARRAY) {
			printf("ERROR: Unable to find build identities node\n");
			return -1;
		}

		for (uint32_t i = 0; i < plist_array_get_size(build_identities_array); i++) {
			plist_t ident = plist_array_get_item(build_identities_array, i);
			if (!ident || plist_get_node_type(ident) != PLIST_DICT) {
				continue;
			}

			plist_t info_dict = plist_dict_get_item(ident, "Info");
			if (!info_dict || plist_get_node_type(info_dict) != PLIST_DICT) {
				continue;
			}

			plist_t build_train = plist_dict_get_item(info_dict, "BuildTrain");
			if (!build_train || plist_get_node_type(build_train) != PLIST_STRING) {
				continue;
			}

			str = NULL;
			plist_get_string_val(build_train, &str);
			break;
		}

		if (str == NULL) {
			printf("ERROR: Could not find Restore image in BuildManifest.\n");
			return 1;
		}

		printf("%s\n", str);
		free(str);
	} else if (options->product == 1) {
		if (options->legacy == 1) {
			plist_t product_type = plist_dict_get_item(root_node, "ProductType");
			if (!product_type || plist_get_node_type(product_type) != PLIST_STRING) {
				printf("ERROR: Unable to find supported products\n");
				return -1;
			}

			str = NULL;
			plist_get_string_val(product_type, &str);

			printf("%s\n", str);
			free(str);
		} else {
			plist_t supported_products = plist_dict_get_item(root_node, "SupportedProductTypes");
			if (!supported_products || plist_get_node_type(supported_products) != PLIST_ARRAY) {
				printf("ERROR: Unable to find supported node\n");
				return -1;
			}

			plist_t product = plist_array_get_item(supported_products, 0);
			if (!product || plist_get_node_type(product) != PLIST_STRING) {
				printf("ERROR: Unable to find supported products\n");
				return -1;
			}

			str = NULL;
			plist_get_string_val(product, &str);

			printf("%s\n", str);
			free(str);
		}
	} else if (options->rootfs == 1) {
		if (options->legacy == 1) {
			plist_t system_restore_images = plist_dict_get_item(root_node, "SystemRestoreImages");
			if (!system_restore_images || plist_get_node_type(system_restore_images) != PLIST_DICT) {
				printf("ERROR: Unable to find system restore images node\n");
				return -1;
			}

			plist_t user = plist_dict_get_item(system_restore_images, "User");
			if (!user || plist_get_node_type(user) != PLIST_STRING) {
				printf("ERROR: Unable to find user restore image\n");
				return -1;
			}

			str = NULL;
			plist_get_string_val(user, &str);

			printf("%s\n", str);
			free(str);
		} else {
			plist_t build_identities_array = plist_dict_get_item(root_node, "BuildIdentities");
			if (!build_identities_array || plist_get_node_type(build_identities_array) != PLIST_ARRAY) {
				printf("ERROR: Unable to find build identities node\n");
				return -1;
			}

			for (uint32_t i = 0; i < plist_array_get_size(build_identities_array); i++) {
				plist_t ident = plist_array_get_item(build_identities_array, i);
				if (!ident || plist_get_node_type(ident) != PLIST_DICT) {
					continue;
				}

				plist_t info_dict = plist_dict_get_item(ident, "Info");
				if (!info_dict || plist_get_node_type(info_dict) != PLIST_DICT) {
					continue;
				}

				if (options->build_train == 1) {
					plist_t build_train = plist_dict_get_item(info_dict, "BuildTrain");
					if (!build_train || plist_get_node_type(build_train) != PLIST_STRING) {
						continue;
					}

					str = NULL;
					plist_get_string_val(build_train, &str);
					break;
				}

				plist_t rbehavior = plist_dict_get_item(info_dict, "RestoreBehavior");
				if (!rbehavior || plist_get_node_type(rbehavior) != PLIST_STRING) {
					continue;
				}

				plist_get_string_val(rbehavior, &str);
				if (strcasecmp(str, "Erase") != 0) {
					str = NULL;
					free(str);
					continue;
				}

				plist_t manifest_dict = plist_dict_get_item(ident, "Manifest");
				if (!manifest_dict || plist_get_node_type(manifest_dict) != PLIST_DICT) {
					continue;
				}

				plist_t os_dict = plist_dict_get_item(manifest_dict, "OS");
				if (!os_dict || plist_get_node_type(os_dict) != PLIST_DICT) {
					continue;
				}

				plist_t os_info_dict = plist_dict_get_item(os_dict, "Info");
				if (!os_info_dict || plist_get_node_type(os_info_dict) != PLIST_DICT) {
					continue;
				}

				plist_t restore_path = plist_dict_get_item(os_info_dict, "Path");
				if (!restore_path || plist_get_node_type(restore_path) != PLIST_STRING) {
					continue;
				}

				str = NULL;
				plist_get_string_val(restore_path, &str);
				break;
			}

			if (str == NULL) {
				printf("ERROR: Could not find Restore image in BuildManifest.\n");
				return 1;
			}

			printf("%s\n", str);
			free(str);
		}
	}

	plist_free(root_node);
	free(plist);

	return 0;
}