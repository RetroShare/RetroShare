/*
 * Copyright (c) 2005-2008 Nominet UK (www.nic.uk)
 * All rights reserved.
 * Contributors: Ben Laurie, Rachel Willmer. The Contributors have asserted
 * their moral rights under the UK Copyright Design and Patents Act 1988 to
 * be recorded as the authors of this copyright work.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. 
 * 
 * You may obtain a copy of the License at 
 *     http://www.apache.org/licenses/LICENSE-2.0 
 * 
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 * 
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define OPS_VERSION_MAJOR	0
/* Set to the version next to be released */
#define OPS_VERSION_MINOR	9
/* 0 for development version, 1 for release */
#define OPS_VERSION_RELEASE	1

#define OPS_VERSION		((OPS_VERSION_MAJOR << 16)+(OPS_VERSION_MINOR << 1)+OPS_VERSION_RELEASE)

#if OPS_VERSION_RELEASE
# define OPS_DEV_STRING ""
#else
# define OPS_DEV_STRING " (dev)"
#endif


#define OPS_VERSION_CAT(a,b)	"OpenPGP:SDK v" #a "." #b OPS_DEV_STRING
#define OPS_VERSION_CAT2(a,b)	OPS_VERSION_CAT(a,b)
#define OPS_VERSION_STRING	OPS_VERSION_CAT2(OPS_VERSION_MAJOR,OPS_VERSION_MINOR)
