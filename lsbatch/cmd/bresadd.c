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
    fprintf(stderr, "usage: bresadd [-V] [-h] res_name res_val\n");
}

int
main(int argc, char **argv)
{
    int cc;
    char *res_name;
    int res_val;
    struct batchRes rsv;

    if (lsb_init(argv[0]) < 0) {
	lsb_perror("lsb_init");
	return -1;
    }

    while ((cc = getopt(argc, argv, "Vh")) != EOF) {
        switch (cc) {
            case 'V':
                fputs(_LS_VERSION_, stderr);
                return 0;
            case 'h':
                usage();
                return 0;
            default:
                break;
        }
    }

    if (argc != 3) {
        usage();
        lsberrno = LSBE_BAD_ARG;
        return -1;
    }

    res_name = strdup(argv[1]);
    if (! isint_(argv[2])) {
        usage();
        lsberrno = LSBE_BAD_ARG;
        return -1;
    }
    res_val = atoi(argv[2]);

    rsv.name = res_name;
    rsv.value = res_val;

    cc = lsb_addbatchres(&rsv);
    if (cc != LSBE_NO_ERROR) {
        lsb_perror("lsb_addbatchres()");
        free(res_name);
        return -1;
    }
    printf("Resource: %s with value %d sent to mbatchd all right.\n",
           res_name, res_val);
    free(res_name);

    return 0;
}
