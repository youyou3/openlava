/*
 * Copyright (C) 2015 David Bigagli
 * Copyright (C) 2007 Platform Computing Inc
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 */

#include "lib.h"

int
getUser(char *userName, unsigned int size)
{
    struct passwd *pw;

    userName[0] = '\0';

    if ((pw = getpwuid(getuid())) == NULL) {
        return LSE_BADUSER;
    }

    if (strlen(pw->pw_name) + 1 > size) {
        return LSE_BAD_ARGS;
    }
    strcpy(userName, pw->pw_name);

    return LSE_NO_ERR;
}

int
getUserByUid(uid_t uid, char *userName, unsigned int size)
{
    struct passwd *pw;

    userName[0] = '\0';

    if ((pw = getpwuid(uid)) == NULL) {
        return LSE_BADUSER;
    }

    if (strlen(pw->pw_name) + 1 > size) {
        return LSE_BAD_ARGS;
    }
    strcpy(userName, pw->pw_name);

    return LSE_NO_ERR;
}

int
getUid(const char *userName, uid_t *uid)
{
    struct passwd *pw;

    if (userName == NULL
        || userName[0] == 0)
        return LSE_BADUSER;

    if ((pw = getpwnam(userName)) == NULL) {
        return LSE_BADUSER;
    }

    *uid = pw->pw_uid;

    return LSE_NO_ERR;
}
