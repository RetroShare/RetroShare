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

#ifndef __OPS_CALLBACK_H__
#define __OPS_CALLBACK_H__

#define CB(cbinfo,t,pc)	do { (pc)->tag=(t); if(ops_parse_cb((pc),(cbinfo)) == OPS_RELEASE_MEMORY) ops_parser_content_free(pc); } while(0)
/*#define CB(cbinfo,t,pc)	do { (pc)->tag=(t); if((cbinfo)->cb(pc,(cbinfo)) == OPS_RELEASE_MEMORY) ops_parser_content_free(pc); } while(0)*/
//#define CB(cbinfo,t,pc)	do { (pc)->tag=(t); if((cbinfo)->cb(pc,(cbinfo)) == OPS_RELEASE_MEMORY) ops_parser_content_free(pc); } while(0)

#define CBP(info,t,pc) CB(&(info)->cbinfo,t,pc)

#define ERR(cbinfo,err,code)	do { content.content.error.error=err; content.tag=OPS_PARSER_ERROR; ops_parse_cb(&content,(cbinfo)); OPS_ERROR(errors,code,err); return -1; } while(0)
 /*#define ERR(err)	do { content.content.error.error=err; content.tag=OPS_PARSER_ERROR; ops_parse_cb(&content,cbinfo); return -1; } while(0)*/

#define ERRP(info,err)	do { C.error.error=err; CBP(info,OPS_PARSER_ERROR,&content); return ops_false; } while(0)


#endif /*__OPS_CALLBACK_H__*/
