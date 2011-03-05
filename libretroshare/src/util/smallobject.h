/*
 * libretroshare/src/util: smallobject.h
 *
 * Universal Networking Header for RetroShare.
 *
 * Copyright 2011 by Cyril Soler
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "cyril.soler@users.sourceforge.net".
 *
 */

#include <stdlib.h>
#include <assert.h>

#include <vector>
#include <map>

#include <util/rsthreads.h>

namespace RsMemoryManagement
{
	static const int MAX_SMALL_OBJECT_SIZE = 128 ;
	static const unsigned char BLOCKS_PER_CHUNK = 255 ;

	struct Chunk
	{
		void init(size_t blockSize,unsigned char blocks);
		void free() ;

		void *allocate(size_t);
		void deallocate(void *p,size_t blockSize);

		unsigned char *_data ;
		unsigned char _firstAvailableBlock ;
		unsigned char _blocksAvailable ;

		void printStatistics(int blockSize) const ;
	};

	class FixedAllocator
	{
		public:
			FixedAllocator(size_t bytes) ;
			virtual ~FixedAllocator() ;

			void *allocate();
			void deallocate(void *p) ;
			inline size_t blockSize() const { return _blockSize ; }
			
			inline bool chunkOwnsPointer(const Chunk& c,void *p) const 
			{ 
				return p >= c._data && (static_cast<unsigned char *>(p)-c._data)/_blockSize < _numBlocks ; 
			}

			void printStatistics() const ;
		private:
			size_t _blockSize ;
			unsigned char _numBlocks ;
			std::vector<Chunk*> _chunks ;
			int _allocChunk ;				// last chunk that provided allocation. -1 if not inited
			int _deallocChunk ;			// last chunk that provided de-allocation. -1 if not inited
	};

	class SmallObjectAllocator
	{
		public:
			SmallObjectAllocator(size_t maxObjectSize) ;
			virtual ~SmallObjectAllocator() ;

			void *allocate(size_t numBytes) ;
			void deallocate(void *p,size_t size) ;

			void printStatistics() const ;

			bool _active ;
		private:
			std::map<int,FixedAllocator*> _pool ;
			FixedAllocator *_lastAlloc ;
			FixedAllocator *_lastDealloc ;
			size_t _maxObjectSize ;
	};

	class SmallObject
	{
		public: 
			static void *operator new(size_t size) ;
			static void operator delete(void *p,size_t size) ;

			static void printStatistics() ;

			virtual ~SmallObject() {}

		private:
			static SmallObjectAllocator _allocator ;
			static RsMutex _mtx;

			friend class SmallObjectAllocator ;
	};

	extern void printStatistics() ;
}


