832c832
<         AuthGPG::getAuthGPG()->GeneratePGPCertificate(name, email, passwd, pgpId, errString);
---
>         return AuthGPG::getAuthGPG()->GeneratePGPCertificate(name, email, passwd, pgpId, errString);
958c958
< 	/* Move directory to correct id */
---
>         /* Move directory to correct id */
1619a1620,1626
> std::string RsInit::RsProfileConfigDirectory()
> {
>     std::string dir = RsInitConfig::basedir + RsInitConfig::dirSeperator + RsInitConfig::preferedId;
>     std::cerr << "RsInit::RsProfileConfigDirectory() returning : " << dir << std::endl;
>     return dir;
> }
> 
1671a1679
> #include "services/p3blogs.h"
1845a1854
> 	std::string blogsdir = config_dir + "/blogs";
1870a1880,1887
> 	
> 			p3Blogs *mBlogs = new p3Blogs(RS_SERVICE_TYPE_QBLOG,
> 			mCacheStrapper, mCacheTransfer, rsFiles,
>                         localcachedir, remotecachedir, blogsdir);
> 
>         CachePair cp6(mBlogs, mBlogs, CacheId(RS_SERVICE_TYPE_QBLOG, 0));
> 	mCacheStrapper -> addCachePair(cp6);
> 	pqih -> addService(mBlogs);  /* This must be also ticked as a service */
1929c1946
< 
---
> 	mConfigMgr->addConfiguration("blogs.cfg", mBlogs);
2045a2063
> 	rsBlogs = mBlogs;
