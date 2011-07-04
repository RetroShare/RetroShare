#include <iostream>
#include "smallobject.h"
#include "util/rsthreads.h"

using namespace RsMemoryManagement ;

RsMutex SmallObject::_mtx("SmallObject") ;
SmallObjectAllocator SmallObject::_allocator(RsMemoryManagement::MAX_SMALL_OBJECT_SIZE) ;

void Chunk::init(size_t blockSize,unsigned char blocks)
{
	_data = new unsigned char[blockSize*blocks] ;
	_firstAvailableBlock = 0 ;
	_blocksAvailable = blocks ;

	// Inits the first byte of each block to point to the next available block
	//
	unsigned char *p = _data ;

	for(unsigned char i=0;i<blocks;p += blockSize)
		*p = ++i ;
}

void Chunk::free()
{
	delete[] _data ;
	_data = NULL ;
}

void *Chunk::allocate(size_t blockSize)
{
	if(!_blocksAvailable)
		return NULL ;

	assert(blockSize >= 2) ;

	unsigned char *result = _data + _firstAvailableBlock*blockSize ;

	// Grab id of next available block from the block being allocated.

	// Always the case because the first available block is the first, so the next
	// available block is given by *result.
	//
	_firstAvailableBlock = *result ; 
	--_blocksAvailable ;

	return result ;
}

void Chunk::deallocate(void *p,size_t blockSize)
{
	assert(p >= _data) ;

	unsigned char *toRelease = static_cast<unsigned char *>(p) ;

	// alignment check
	
	assert( (toRelease - _data) % blockSize == 0 ) ;

	*toRelease = _firstAvailableBlock ;
	_firstAvailableBlock = static_cast<unsigned char>( (toRelease - _data)/blockSize) ;

	// truncation check
	
	assert(_firstAvailableBlock == (toRelease - _data)/blockSize);

	++_blocksAvailable ;
}

void Chunk::printStatistics(int blockSize) const
{
	std::cerr << "      blocksAvailable    : " << (int)_blocksAvailable << std::endl;
	std::cerr << "      firstBlockAvailable: " << (int)_firstAvailableBlock << std::endl;
	std::cerr << "      blocks             : " << (void*)_data << " to " << (void*)(_data+BLOCKS_PER_CHUNK*blockSize) << std::endl;
}

FixedAllocator::FixedAllocator(size_t bytes)
{
	_blockSize = bytes ;
	_numBlocks = BLOCKS_PER_CHUNK ;
	_allocChunk = -1 ;
	_deallocChunk = -1 ;
}
FixedAllocator::~FixedAllocator()
{
	for(uint32_t i=0;i<_chunks.size();++i)
	{
		_chunks[i]->free() ;
		delete _chunks[i] ;
	}
}

void *FixedAllocator::allocate()
{
	if(_allocChunk < 0 || _chunks[_allocChunk]->_blocksAvailable == 0)
	{
		// find availabel memory in this chunk
		//
		uint32_t i ;
		_allocChunk = -1 ;
		for(i=0;i<_chunks.size();++i)
			if(_chunks[i]->_blocksAvailable > 0)	// found a chunk
			{
				_allocChunk = i ;
				break ;
			}
		if( _allocChunk < 0 )
		{
			_chunks.reserve(_chunks.size()+1) ;
			Chunk *newChunk = new Chunk;

			if(newChunk == NULL)
			{
				std::cerr << "RsMemoryManagement: ran out of memory !" << std::endl;
				exit(-1) ;
			}
			newChunk->init(_blockSize,_numBlocks) ;
			_chunks.push_back(newChunk) ;

			_allocChunk = _chunks.size()-1 ;
			_deallocChunk = _chunks.size()-1 ;
		}

	}
	assert(_chunks[_allocChunk] != NULL) ;
	assert(_chunks[_allocChunk]->_blocksAvailable > 0) ;

	return _chunks[_allocChunk]->allocate(_blockSize) ;
}
void FixedAllocator::deallocate(void *p)
{
	if(_deallocChunk < 0 || !chunkOwnsPointer(*_chunks[_deallocChunk],p))
	{
		// find the chunk that contains this pointer. Perform a linear search.

		_deallocChunk = -1 ;

		for(uint32_t i=0;i<_chunks.size();++i)
			if(chunkOwnsPointer(*_chunks[i],p))
			{
				_deallocChunk = i ;
				break ;
			}
	}
	assert(_chunks[_deallocChunk] != NULL) ;

	_chunks[_deallocChunk]->deallocate(p,_blockSize) ;

	if(_chunks[_deallocChunk]->_blocksAvailable == BLOCKS_PER_CHUNK)
	{
		_chunks[_deallocChunk]->free() ;
		delete _chunks[_deallocChunk] ;

		_chunks[_deallocChunk] = _chunks.back() ;
		if(_allocChunk == _deallocChunk) _allocChunk = -1 ;
		if(_allocChunk == ((int)_chunks.size())-1) _allocChunk = _deallocChunk ;
		_deallocChunk = -1 ;
		_chunks.pop_back();
	}
}
void FixedAllocator::printStatistics() const
{
	std::cerr << "    numBLocks=" << (int)_numBlocks << std::endl;
	std::cerr << "    blockSize=" << (int)_blockSize << std::endl;
	std::cerr << "    Number of chunks: " << _chunks.size() << std::endl;
	for(uint32_t i=0;i<_chunks.size();++i)
		_chunks[i]->printStatistics(_blockSize) ;
}

SmallObjectAllocator::SmallObjectAllocator(size_t maxObjectSize)
	: _maxObjectSize(maxObjectSize)
{
	RsStackMutex m(SmallObject::_mtx) ;

	_lastAlloc = NULL ;
	_lastDealloc = NULL ;
	_active = true ;
}

SmallObjectAllocator::~SmallObjectAllocator()
{
	RsStackMutex m(SmallObject::_mtx) ;

	for(std::map<int,FixedAllocator*>::const_iterator it(_pool.begin());it!=_pool.end();++it)
		delete it->second ;

	_active = false ;
}

void *SmallObjectAllocator::allocate(size_t bytes)
{
	if(bytes > _maxObjectSize)
		return malloc(bytes) ;
	else if(_lastAlloc != NULL && _lastAlloc->blockSize() == bytes)
		return _lastAlloc->allocate() ;
	else
	{
		std::map<int,FixedAllocator*>::iterator it(_pool.find(bytes)) ;

		if(it == _pool.end())
		{
			_pool[bytes] = new FixedAllocator(bytes) ;
			it = _pool.find(bytes) ;
		}
		_lastAlloc = it->second ;

		return it->second->allocate() ;
	}
}

void SmallObjectAllocator::deallocate(void *p,size_t bytes)
{
	if(bytes > _maxObjectSize)
		free(p) ;
	else if(_lastDealloc != NULL && _lastDealloc->blockSize() == bytes)
		_lastDealloc->deallocate(p) ;
	else
	{
		std::map<int,FixedAllocator*>::iterator it(_pool.find(bytes)) ;

		if(it == _pool.end())
		{
			_pool[bytes] = new FixedAllocator(bytes) ;
			it = _pool.find(bytes) ;
		}
		it->second->deallocate(p) ;
		_lastDealloc = it->second ;
	}
}

void SmallObjectAllocator::printStatistics() const
{
	std::cerr << "RsMemoryManagement Statistics:" << std::endl;
	std::cerr << "  Total Fixed-size allocators: " << _pool.size() << std::endl;
	std::cerr << "Pool" << std::endl;

	for(std::map<int,FixedAllocator*>::const_iterator it(_pool.begin());it!=_pool.end();++it)
	{
		std::cerr << "  Allocator for size " << it->first << " : " << std::endl;
		std::cerr << "  Last Alloc: " << _lastAlloc << std::endl;
		std::cerr << "  Last Dealloc: " << _lastDealloc << std::endl;
		it->second->printStatistics() ;
	}
}

void *SmallObject::operator new(size_t size)
{
#ifdef DEBUG_MEMORY
	bool print=false ;
	{
		RsStackMutex m(_mtx) ;
		static time_t last_time = 0 ;
		time_t now = time(NULL) ;
		if(now > last_time + 20)
		{
			last_time = now ;
			print=true ;
		}
	}
	if(print)
		printStatistics() ;
#endif

	RsStackMutex m(_mtx) ;

	if(!_allocator._active)
		return (void*)NULL;

	void *p = _allocator.allocate(size) ;
#ifdef DEBUG_MEMORY
	std::cerr << "new RsItem: " << p << ", size=" << size << std::endl;
#endif
	return p ;
}

void SmallObject::operator delete(void *p,size_t size)
{
	RsStackMutex m(_mtx) ;

	if(!_allocator._active)
		return ;

	_allocator.deallocate(p,size) ;
#ifdef DEBUG_MEMORY
	std::cerr << "del RsItem: " << p << ", size=" << size << std::endl;
#endif
}

void SmallObject::printStatistics() 
{
	RsStackMutex m(_mtx) ;

	if(!_allocator._active)
		return ;

	_allocator.printStatistics() ;
}

void RsMemoryManagement::printStatistics()
{
	SmallObject::printStatistics();
}


