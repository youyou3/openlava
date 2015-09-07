/*
 * Copyright (C) 2015 David Bigagli
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA
 *
 */

#include "cmd.h"

void
usage(void)
{
    fprintf(stderr, "\
usage: bresadd [-V] [-h] -r res_name -v res_val [-f]\n");
}

int
main(int argc, char **argv)
{
    int cc;
    struct batchRes rsv;

    if (lsb_init(argv[0]) < 0) {
	lsb_perror("lsb_init");
	return -1;
    }

    memset(&rsv, 0, sizeof(struct batchRes));
    while ((cc = getopt(argc, argv, "Vhfr:v:")) != EOF) {
        switch (cc) {
            case 'V':
                fputs(_LS_VERSION_, stderr);
                return 0;
            case 'h':
                usage();
                return 0;
            case 'r':
                rsv.name = strdup(optarg);
                break;
            case 'v':
                rsv.value = atoi(optarg);
                break;
            case 'f':
                rsv.options |= RES_RM_FORCE;
                break;
            default:
                break;
        }
    }

    if (rsv.name == NULL) {
        lsberrno = LSBE_BAD_ARG;
        usage();
        return -1;
    }

    cc = lsb_rmbatchres(&rsv);
    if (cc != LSBE_NO_ERROR) {
        lsb_perror("lsb_rmbatchres()");
        free(rsv.name);
        return -1;
    }

    if (rsv.options & RES_RM_FORCE) {
        printf("\
Resource: %s with value %d removed with RES_RM_FORCE from mbatchd all right.\n",
               rsv.name, rsv.value);
    } else {
        printf("\
Resource: %s with value %d removed from mbatchd all right.\n",
               rsv.name, rsv.value);
    }

    free(rsv.name);

    return 0;
}
