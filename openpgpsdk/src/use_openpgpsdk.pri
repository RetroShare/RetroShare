#
# Copyright (C) 2018, Retroshare team <retroshare.team@gmailcom>
# All rights reserved.
# Contributors: retroshare team. The Contributors have asserted
# their moral rights under the UK Copyright Design and Patents Act 1988 to
# be recorded as the authors of this copyright work.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.
#
# You may obtain a copy of the License at
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# See the License for the specific language governing permissions and
# limitations under the License.
#
DEPENDPATH *= $$system_path($$clean_path($${PWD}/../../openpgpsdk/src))
INCLUDEPATH  *= $$system_path($$clean_path($${PWD}/../../openpgpsdk/src))
LIBS *= -L$$system_path($$clean_path($${OUT_PWD}/../../openpgpsdk/src/lib/)) -lops

!equals(TARGET, ops):PRE_TARGETDEPS *= $$system_path($$clean_path($${OUT_PWD}/../../openpgpsdk/src/lib/libops.a))

sLibs =
mLibs = ssl crypto z bz2
dLibs =

static {
    sLibs *= $$mLibs
} else {
    dLibs *= $$mLibs
}

LIBS += $$linkStaticLibs(sLibs)
PRE_TARGETDEPS += $$pretargetStaticLibs(sLibs)

LIBS += $$linkDynamicLibs(dLibs)
