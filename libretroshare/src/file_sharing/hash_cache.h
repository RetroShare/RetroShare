class HashCache
{
    public:
        HashCache(const std::string& save_file_name) ;

        void save() ;
        void insert(const std::string& full_path,uint64_t size,time_t time_stamp,const RsFileHash& hash) ;
        bool find(const  std::string& full_path,uint64_t size,time_t time_stamp,RsFileHash& hash) ;
        void clean() ;

        typedef struct
        {
            uint64_t size ;
            uint64_t time_stamp ;
            uint64_t modf_stamp ;
            RsFileHash hash ;
        } HashCacheInfo ;

        void setRememberHashFilesDuration(uint32_t days) { _max_cache_duration_days = days ; }
        uint32_t rememberHashFilesDuration() const { return _max_cache_duration_days ; }
        void clear() { _files.clear(); }
        bool empty() const { return _files.empty() ; }
    private:
        uint32_t _max_cache_duration_days ; // maximum duration of un-requested cache entries
        std::map<std::string, HashCacheInfo> _files ;
        std::string _path ;
        bool _changed ;
};

