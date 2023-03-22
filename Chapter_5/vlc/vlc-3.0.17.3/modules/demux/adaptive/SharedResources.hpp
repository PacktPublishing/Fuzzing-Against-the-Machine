/*
 * SharedResources.h
 *****************************************************************************
 * Copyright © 2019 VideoLabs, VideoLAN and VLC Authors
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
#ifndef SHAREDRESOURCES_H_
#define SHAREDRESOURCES_H_

#include <vlc_common.h>
#include <string>

namespace adaptive
{
    namespace http
    {
        class AuthStorage;
        class AbstractConnectionManager;
    }

    namespace encryption
    {
        class Keyring;
    }

    using namespace http;
    using namespace encryption;

    class SharedResources
    {
        public:
            SharedResources(AuthStorage *, Keyring *, AbstractConnectionManager *);
            ~SharedResources();
            AuthStorage *getAuthStorage();
            Keyring     *getKeyring();
            AbstractConnectionManager *getConnManager();
            /* Helper */
            static SharedResources * createDefault(vlc_object_t *, const std::string &);

        private:
            AuthStorage *authStorage;
            Keyring *encryptionKeyring;
            AbstractConnectionManager *connManager;
    };
}

#endif
