#pragma once
#include "TUtilities.h"

#include <iterator>

// Disable the DLL- linkage warning for now
#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(disable : 4251)
#endif

namespace LambdaEngine
{
	/*
	* Dynamic Array similar to std::vector
	*/
	template<typename T>
	class TArray
	{
	public:
		typedef uint32 SizeType;

		/*
		* IteratorBase
		*/
		template<typename TIteratorType>
		class IteratorBase
		{
			friend class TArray;

		public:
			using iterator_category = std::bidirectional_iterator_tag;
			using difference_type	= SizeType;
			using value_type		= TIteratorType;
			using pointer			= TIteratorType*;
			using reference			= TIteratorType&;

			~IteratorBase() = default;

			FORCEINLINE IteratorBase(TIteratorType* ptr = nullptr)
				: m_Ptr(ptr)
			{
			}

			FORCEINLINE TIteratorType* operator->() const
			{
				return m_Ptr;
			}

			FORCEINLINE TIteratorType& operator*() const
			{
				return *m_Ptr;
			}

			FORCEINLINE IteratorBase operator++()
			{
				m_Ptr++;
				return *this;
			}

			FORCEINLINE IteratorBase operator++(int32)
			{
				IteratorBase Temp = *this;
				m_Ptr++;
				return Temp;
			}

			FORCEINLINE IteratorBase operator--()
			{
				m_Ptr--;
				return *this;
			}

			FORCEINLINE IteratorBase operator--(int32)
			{
				IteratorBase Temp = *this;
				m_Ptr--;
				return Temp;
			}

			FORCEINLINE IteratorBase operator+(int32 Offset) const
			{
				IteratorBase Temp = *this;
				return Temp += Offset;
			}

			FORCEINLINE IteratorBase operator-(int32 Offset) const
			{
				IteratorBase Temp = *this;
				return Temp -= Offset;
			}

			FORCEINLINE IteratorBase& operator+=(int32 Offset)
			{
				m_Ptr += Offset;
				return *this;
			}

			FORCEINLINE IteratorBase& operator-=(int32 Offset)
			{
				m_Ptr -= Offset;
				return *this;
			}

			FORCEINLINE bool operator==(const IteratorBase& other) const
			{
				return (m_Ptr == other.m_Ptr);
			}

			FORCEINLINE bool operator!=(const IteratorBase& other) const
			{
				return (m_Ptr != other.m_Ptr);
			}

			FORCEINLINE bool operator<(const IteratorBase& other) const
			{
				return m_Ptr < other.m_Ptr;
			}

			FORCEINLINE bool operator<=(const IteratorBase& other) const
			{
				return m_Ptr <= other.m_Ptr;
			}

			FORCEINLINE bool operator>(const IteratorBase& other) const
			{
				return m_Ptr > other.m_Ptr;
			}

			FORCEINLINE bool operator>=(const IteratorBase& other) const
			{
				return m_Ptr >= other.m_Ptr;
			}

		protected:
			TIteratorType* m_Ptr;
		};

		typedef IteratorBase<T>			Iterator;
		typedef IteratorBase<const T>	ConstIterator;

		/*
		* Reverse Iterator: Stores for example End(), but will reference End() - 1
		*/
		template<typename TIteratorType>
		class ReverseIteratorBase
		{
			friend class TArray;

		public:
			~ReverseIteratorBase() = default;

			FORCEINLINE ReverseIteratorBase(TIteratorType* ptr = nullptr)
				: m_Ptr(ptr)
			{
			}

			FORCEINLINE TIteratorType* operator->() const
			{
				return (m_Ptr - 1);
			}

			FORCEINLINE TIteratorType& operator*() const
			{
				return *(m_Ptr - 1);
			}

			FORCEINLINE ReverseIteratorBase operator++()
			{
				m_Ptr--;
				return *this;
			}

			FORCEINLINE ReverseIteratorBase operator++(int32)
			{
				ReverseIteratorBase Temp = *this;
				m_Ptr--;
				return Temp;
			}

			FORCEINLINE ReverseIteratorBase operator--()
			{
				m_Ptr++;
				return *this;
			}

			FORCEINLINE ReverseIteratorBase operator--(int32)
			{
				ReverseIteratorBase Temp = *this;
				m_Ptr++;
				return Temp;
			}

			FORCEINLINE ReverseIteratorBase operator+(int32 Offset) const
			{
				ReverseIteratorBase Temp = *this;
				return Temp += Offset;
			}

			FORCEINLINE ReverseIteratorBase operator-(int32 Offset) const
			{
				ReverseIteratorBase Temp = *this;
				return Temp -= Offset;
			}

			FORCEINLINE ReverseIteratorBase& operator+=(int32 Offset)
			{
				m_Ptr -= Offset;
				return *this;
			}

			FORCEINLINE ReverseIteratorBase& operator-=(int32 Offset)
			{
				m_Ptr += Offset;
				return *this;
			}

			FORCEINLINE bool operator==(const ReverseIteratorBase& other) const
			{
				return (m_Ptr == other.m_Ptr);
			}

			FORCEINLINE bool operator!=(const ReverseIteratorBase& other) const
			{
				return (m_Ptr != other.m_Ptr);
			}

		protected:
			TIteratorType* m_Ptr;
		};

		typedef ReverseIteratorBase<T>			ReverseIterator;
		typedef ReverseIteratorBase<const T>	ReverseConstIterator;

	/*
	* TArray API
	*/
	public:
		FORCEINLINE TArray() noexcept
			: m_pData(nullptr)
			, m_Size(0)
			, m_Capacity(0)
		{
		}

		FORCEINLINE explicit TArray(SizeType size) noexcept
			: m_pData(nullptr)
			, m_Size(0)
			, m_Capacity(0)
		{
			InternalConstruct(size);
		}

		FORCEINLINE explicit TArray(SizeType size, const T& value) noexcept
			: m_pData(nullptr)
			, m_Size(0)
			, m_Capacity(0)
		{
			InternalConstruct(size, value);
		}

		template<typename TInputIt>
		FORCEINLINE explicit TArray(TInputIt begin, TInputIt end) noexcept
			: m_pData(nullptr)
			, m_Size(0)
			, m_Capacity(0)
		{
			InternalConstruct(begin, end);
		}

		FORCEINLINE TArray(std::initializer_list<T> iList) noexcept
			: m_pData(nullptr)
			, m_Size(0)
			, m_Capacity(0)
		{
			InternalConstruct(iList.begin(), iList.end());
		}

		FORCEINLINE TArray(const TArray& other) noexcept
			: m_pData(nullptr)
			, m_Size(0)
			, m_Capacity(0)
		{
			InternalConstruct(other.Begin(), other.End());
		}

		FORCEINLINE TArray(TArray&& other) noexcept
			: m_pData(nullptr)
			, m_Size(0)
			, m_Capacity(0)
		{
			InternalMove(Move(other));
		}

		FORCEINLINE ~TArray()
		{
			Clear();

			InternalReleaseData();

			m_Capacity = 0;
			m_pData = nullptr;
		}

		FORCEINLINE void Clear() noexcept
		{
			InternalDestructRange(m_pData, m_pData + m_Size);
			m_Size = 0;
		}

		FORCEINLINE void Assign(SizeType size) noexcept
		{
			Clear();
			InternalConstruct(size);
		}

		FORCEINLINE void Assign(SizeType size, const T& value) noexcept
		{
			Clear();
			InternalConstruct(size, value);
		}

		template<typename TInputIt>
		FORCEINLINE void Assign(TInputIt begin, TInputIt end) noexcept
		{
			Clear();
			InternalConstruct(begin, end);
		}

		FORCEINLINE void Assign(std::initializer_list<T> iList) noexcept
		{
			Clear();
			InternalConstruct(iList.begin(), iList.end());
		}

		FORCEINLINE void Resize(SizeType size) noexcept
		{
			if (size > m_Size)
			{
				if (size >= m_Capacity)
				{
					const SizeType newCapacity = InternalGetResizeFactor(size);
					InternalRealloc(newCapacity);
				}

				InternalDefaultConstructRange(m_pData + m_Size, m_pData + size);
			}
			else if (size < m_Size)
			{
				InternalDestructRange(m_pData + size, m_pData + m_Size);
			}

			m_Size = size;
		}

		FORCEINLINE void Resize(SizeType size, const T& value) noexcept
		{
			if (size > m_Size)
			{
				if (size >= m_Capacity)
				{
					const SizeType newCapacity = InternalGetResizeFactor(size);
					InternalRealloc(newCapacity);
				}

				InternalCopyEmplace(size - m_Size, value, m_pData + m_Size);
			}
			else if (size < m_Size)
			{
				InternalDestructRange(m_pData + size, m_pData + m_Size);
			}

			m_Size = size;
		}

		FORCEINLINE void Reserve(SizeType inCapacity) noexcept
		{
			if (inCapacity != m_Capacity)
			{
				SizeType oldSize = m_Size;
				if (inCapacity < m_Size)
				{
					m_Size = inCapacity;
				}

				T* tempData = InternalAllocateElements(inCapacity);
				InternalMoveEmplace(m_pData, m_pData + m_Size, tempData);
				InternalDestructRange(m_pData, m_pData + oldSize);
				InternalReleaseData();

				m_pData = tempData;
				m_Capacity = inCapacity;
			}
		}

		template<typename... TArgs>
		FORCEINLINE T& EmplaceBack(TArgs&&... args) noexcept
		{
			if (m_Size >= m_Capacity)
			{
				const SizeType newCapacity = InternalGetResizeFactor();
				InternalRealloc(newCapacity);
			}

			T* dataEnd = m_pData + m_Size;
			new(reinterpret_cast<void*>(dataEnd)) T(Forward<TArgs>(args)...);
			m_Size++;
			return (*dataEnd);
		}

		FORCEINLINE T& PushBack(const T& element) noexcept
		{
			return EmplaceBack(element);
		}

		FORCEINLINE T& PushBack(T&& element) noexcept
		{
			return EmplaceBack(Move(element));
		}

		template<typename... TArgs>
		FORCEINLINE Iterator Emplace(ConstIterator pos, TArgs&&... args) noexcept
		{
			// Emplace back
			if (pos == ConstEnd())
			{
				const SizeType oldSize = m_Size;
				EmplaceBack(Forward<TArgs>(args)...);
				return Iterator(m_pData + oldSize);
			}

			// Emplace
			const SizeType index = InternalIndex(pos);
			T* dataBegin = m_pData + index;
			if (m_Size >= m_Capacity)
			{
				const SizeType newCapacity = InternalGetResizeFactor();
				InternalEmplaceRealloc(newCapacity, dataBegin, 1);
				dataBegin = m_pData + index;
			}
			else
			{
				// Construct the range so that we can move to it
				T* dataEnd = m_pData + m_Size;
				InternalDefaultConstructRange(dataEnd, dataEnd + 1);
				InternalMemmoveForward(dataBegin, dataEnd, dataEnd);
				InternalDestruct(dataBegin);
			}

			new (reinterpret_cast<void*>(dataBegin)) T(Forward<TArgs>(args)...);
			m_Size++;
			return Iterator(dataBegin);
		}

		FORCEINLINE Iterator Insert(Iterator pos, const T& value) noexcept
		{
			return Emplace(ConstIterator(pos.m_Ptr), value);
		}

		FORCEINLINE Iterator Insert(Iterator pos, T&& value) noexcept
		{
			return Emplace(ConstIterator(pos.m_Ptr), Move(value));
		}

		FORCEINLINE Iterator Insert(ConstIterator pos, const T& value) noexcept
		{
			return Emplace(pos, value);
		}

		FORCEINLINE Iterator Insert(ConstIterator pos, T&& value) noexcept
		{
			return Emplace(pos, Move(value));
		}

		FORCEINLINE Iterator Insert(Iterator pos, std::initializer_list<T> iList) noexcept
		{
			return Insert(Iterator(pos.m_Ptr), iList);
		}

		FORCEINLINE Iterator Insert(ConstIterator pos, std::initializer_list<T> iList) noexcept
		{
			// Insert at end
			if (pos == ConstEnd())
			{
				const SizeType oldSize = m_Size;
				for (const T& value : iList)
				{
					EmplaceBack(Move(value));
				}

				return Iterator(m_pData + oldSize);
			}

			// Insert
			const SizeType listSize	= static_cast<SizeType>(iList.size());
			const SizeType newSize	= m_Size + listSize;
			const SizeType index	= InternalIndex(pos);

			T* rangeBegin = m_pData + index;
			if (newSize >= m_Capacity)
			{
				const SizeType newCapacity = InternalGetResizeFactor(newSize);
				InternalEmplaceRealloc(newCapacity, rangeBegin, listSize);
				rangeBegin = m_pData + index;
			}
			else
			{
				// Construct the range so that we can move to it
				T* dataEnd		= m_pData + m_Size;
				T* newDataEnd	= m_pData + m_Size + listSize;
				T* rangeEnd		= rangeBegin + listSize;
				InternalDefaultConstructRange(dataEnd, newDataEnd);
				InternalMemmoveForward(rangeBegin, dataEnd, newDataEnd - 1);
				InternalDestructRange(rangeBegin, rangeEnd);
			}

			// TODO: Get rid of const_cast
			InternalMoveEmplace(const_cast<T*>(iList.begin()), const_cast<T*>(iList.end()), rangeBegin);
			m_Size = newSize;
			return Iterator(rangeBegin);
		}

		template<typename TInputIt>
		FORCEINLINE Iterator Insert(Iterator pos, TInputIt begin, TInputIt end) noexcept
		{
			return Insert(ConstIterator(pos.m_Ptr), begin, end);
		}

		template<typename TInputIt>
		FORCEINLINE Iterator Insert(ConstIterator pos, TInputIt begin, TInputIt end) noexcept
		{
			// Insert at end
			if (pos == ConstEnd())
			{
				const SizeType oldSize = m_Size;
				for (TInputIt it = begin; it != end; it++)
				{
					EmplaceBack(*it);
				}

				return Iterator(m_pData + oldSize);
			}

			// Insert
			const SizeType rangeSize	= InternalDistance(begin.m_Ptr, end.m_Ptr);
			const SizeType newSize		= m_Size + rangeSize;
			const SizeType index		= InternalIndex(pos);

			T* rangeBegin = m_pData + index;
			if (newSize >= m_Capacity)
			{
				const SizeType newCapacity = InternalGetResizeFactor(newSize);
				InternalEmplaceRealloc(newCapacity, rangeBegin, rangeSize);
				rangeBegin = m_pData + index;
			}
			else
			{
				// Construct the range so that we can move to it
				T* dataEnd		= m_pData + m_Size;
				T* newDataEnd	= m_pData + m_Size + rangeSize;
				T* rangeEnd		= rangeBegin + rangeSize;
				InternalDefaultConstructRange(dataEnd, newDataEnd);
				InternalMemmoveForward(rangeBegin, dataEnd, newDataEnd - 1);
				InternalDestructRange(rangeBegin, rangeEnd);
			}

			InternalCopyEmplace(begin.m_Ptr, end.m_Ptr, rangeBegin);
			m_Size = newSize;
			return Iterator(rangeBegin);
		}

		FORCEINLINE void PopBack() noexcept
		{
			if (m_Size > 0)
			{
				InternalDestruct(m_pData + (--m_Size));
			}
		}

		FORCEINLINE Iterator Erase(Iterator pos) noexcept
		{
			return Erase(ConstIterator(pos.m_Ptr));
		}

		FORCEINLINE Iterator Erase(ConstIterator pos) noexcept
		{
			VALIDATE(InternalIsIteratorOwner(pos));

			// Erase at end
			if (pos == ConstEnd())
			{
				PopBack();
				return End();
			}

			// Erase
			const T* constData = m_pData;
			const SizeType index = InternalDistance(constData, pos.m_Ptr);
			T* dataBegin	= m_pData + index;
			T* dataEnd		= m_pData + m_Size;
			InternalMemmoveBackwards(dataBegin + 1, dataEnd, dataBegin);
			InternalDestruct(dataEnd - 1);

			m_Size--;
			return Iterator(dataBegin);
		}

		FORCEINLINE Iterator Erase(Iterator begin, Iterator end) noexcept
		{
			return Erase(ConstIterator(begin.m_Ptr), ConstIterator(end.m_Ptr));
		}

		FORCEINLINE Iterator Erase(ConstIterator begin, ConstIterator end) noexcept
		{
			VALIDATE(begin < end);
			VALIDATE(InternalIsRangeOwner(begin, end));

			T* dataBegin	= m_pData + InternalIndex(begin);
			T* dataEnd		= m_pData + InternalIndex(end);
			
			const SizeType elementCount = InternalDistance(dataBegin, dataEnd);
			if (end >= ConstEnd())
			{
				InternalDestructRange(dataBegin, dataEnd);
			}
			else
			{
				T* realEnd = m_pData + m_Size;
				InternalMemmoveBackwards(dataEnd, realEnd, dataBegin);
				InternalDestructRange(realEnd - elementCount, realEnd);
			}

			m_Size -= elementCount;
			return Iterator(dataBegin);
		}

		FORCEINLINE void Swap(TArray& other) noexcept
		{
			if (this != std::addressof(other))
			{
				T* tempPtr = m_pData;
				SizeType tempSize = m_Size;
				SizeType tempCapacity = m_Capacity;

				m_pData = other.m_pData;
				m_Size = other.m_Size;
				m_Capacity = other.m_Capacity;

				other.m_pData = tempPtr;
				other.m_Size = tempSize;
				other.m_Capacity = tempCapacity;
			}
		}

		FORCEINLINE void ShrinkToFit() noexcept
		{
			if (m_Capacity > m_Size)
			{
				InternalRealloc(m_Size);
			}
		}

		FORCEINLINE bool IsEmpty() const noexcept
		{
			return (m_Size == 0);
		}

		FORCEINLINE Iterator Begin() noexcept
		{
			return Iterator(m_pData);
		}

		FORCEINLINE Iterator End() noexcept
		{
			return Iterator(m_pData + m_Size);
		}

		FORCEINLINE ConstIterator Begin() const noexcept
		{
			return ConstIterator(m_pData);
		}

		FORCEINLINE ConstIterator End() const noexcept
		{
			return ConstIterator(m_pData + m_Size);
		}

		FORCEINLINE ConstIterator ConstBegin() const noexcept
		{
			return ConstIterator(m_pData);
		}

		FORCEINLINE ConstIterator ConstEnd() const noexcept
		{
			return ConstIterator(m_pData + m_Size);
		}

		FORCEINLINE ReverseIterator ReverseBegin() noexcept
		{
			return ReverseIterator(m_pData + m_Size);
		}

		FORCEINLINE ReverseIterator ReverseEnd() noexcept
		{
			return ReverseIterator(m_pData);
		}

		FORCEINLINE ReverseConstIterator ReverseBegin() const noexcept
		{
			return ReverseConstIterator(m_pData + m_Size);
		}

		FORCEINLINE ReverseConstIterator ReverseEnd() const noexcept
		{
			return ReverseConstIterator(m_pData);
		}

		FORCEINLINE ReverseConstIterator ConstReverseBegin() const noexcept
		{
			return ReverseConstIterator(m_pData + m_Size);
		}

		FORCEINLINE ReverseConstIterator ConstReverseEnd() const noexcept
		{
			return ReverseConstIterator(m_pData);
		}

		FORCEINLINE T& GetFront() noexcept
		{
			VALIDATE(m_Size > 0);
			return m_pData[0];
		}

		FORCEINLINE const T& GetFront() const noexcept
		{
			VALIDATE(m_Size > 0);
			return m_pData[0];
		}

		FORCEINLINE T& GetBack() noexcept
		{
			VALIDATE(m_Size > 0);
			return m_pData[m_Size - 1];
		}

		FORCEINLINE const T& GetBack() const noexcept
		{
			VALIDATE(m_Size > 0);
			return m_pData[m_Size - 1];
		}

		FORCEINLINE T* GetData() noexcept
		{
			return m_pData;
		}

		FORCEINLINE const T* GetData() const noexcept
		{
			return m_pData;
		}

		FORCEINLINE SizeType GetSize() const noexcept
		{
			return m_Size;
		}

		FORCEINLINE SizeType GetCapacity() const noexcept
		{
			return m_Capacity;
		}

		FORCEINLINE T& GetElementAt(SizeType index) noexcept
		{
			VALIDATE(index < m_Size);
			return m_pData[index];
		}

		FORCEINLINE const T& GetElementAt(SizeType index) const noexcept
		{
			VALIDATE(index < m_Size);
			return m_pData[index];
		}

		FORCEINLINE TArray& operator=(const TArray& other) noexcept
		{
			if (this != std::addressof(other))
			{
				Clear();
				InternalConstruct(other.Begin(), other.End());
			}

			return *this;
		}

		FORCEINLINE TArray& operator=(TArray&& other) noexcept
		{
			if (this != std::addressof(other))
			{
				Clear();
				InternalReleaseData();
				InternalMove(Move(other));
			}

			return *this;
		}

		FORCEINLINE TArray& operator=(std::initializer_list<T> iList) noexcept
		{
			Assign(iList);
			return *this;
		}

		FORCEINLINE T& operator[](SizeType index) noexcept
		{
			return GetElementAt(index);
		}

		FORCEINLINE const T& operator[](SizeType index) const noexcept
		{
			return GetElementAt(index);
		}

	/*
	* STL iterator functions, Only here so that you can use Range for-loops
	*/
	public:
		FORCEINLINE Iterator begin() noexcept
		{
			return Iterator(m_pData);
		}

		FORCEINLINE Iterator end() noexcept
		{
			return Iterator(m_pData + m_Size);
		}

		FORCEINLINE ConstIterator begin() const noexcept
		{
			return ConstIterator(m_pData);
		}

		FORCEINLINE ConstIterator end() const noexcept
		{
			return ConstIterator(m_pData + m_Size);
		}

		FORCEINLINE ConstIterator cbegin() const noexcept
		{
			return ConstIterator(m_pData);
		}

		FORCEINLINE ConstIterator cend() const noexcept
		{
			return ConstIterator(m_pData + m_Size);
		}

		FORCEINLINE ReverseIterator rbegin() noexcept
		{
			return ReverseIterator(m_pData);
		}

		FORCEINLINE ReverseIterator rend() noexcept
		{
			return ReverseIterator(m_pData + m_Size);
		}

		FORCEINLINE ReverseConstIterator rbegin() const noexcept
		{
			return ReverseConstIterator(m_pData);
		}

		FORCEINLINE ReverseConstIterator rend() const noexcept
		{
			return ReverseConstIterator(m_pData + m_Size);
		}

		FORCEINLINE ReverseConstIterator crbegin() const noexcept
		{
			return ReverseConstIterator(m_pData);
		}

		FORCEINLINE ReverseConstIterator crend() const noexcept
		{
			return ReverseConstIterator(m_pData + m_Size);
		}

	private:
		// Check is the iterator belongs to this TArray
		FORCEINLINE bool InternalIsRangeOwner(ConstIterator begin, ConstIterator end)
		{
			return (begin < end) && (begin >= ConstBegin()) && (end <= ConstEnd());
		}

		FORCEINLINE bool InternalIsIteratorOwner(ConstIterator it)
		{
			return (it >= ConstBegin()) && (it <= ConstEnd());
		}

		// Helpers
		template<typename TInputIt>
		FORCEINLINE T* InternalUnwrap(TInputIt it)
		{
			if constexpr (std::is_pointer<TInputIt>())
			{
				return it;
			}
			else
			{
				return it.m_Ptr;
			}
		}

		template<typename TInputIt>
		FORCEINLINE const T* InternalUnwrapConst(TInputIt it)
		{
			if constexpr (std::is_pointer<TInputIt>())
			{
				return it;
			}
			else
			{
				return it.m_Ptr;
			}
		}

		template<typename TInputIt>
		FORCEINLINE SizeType InternalDistance(TInputIt begin, TInputIt end)
		{
			return static_cast<SizeType>(InternalUnwrapConst(end) - InternalUnwrapConst(begin));
		}

		template<typename TInputIt>
		FORCEINLINE SizeType InternalIndex(TInputIt pos)
		{
			const T* constData = m_pData;
			return static_cast<SizeType>(InternalUnwrapConst(pos) - constData);
		}

		FORCEINLINE SizeType InternalGetResizeFactor() const
		{
			return InternalGetResizeFactor(m_Size);
		}

		FORCEINLINE SizeType InternalGetResizeFactor(SizeType baseSize) const
		{
			return baseSize + (m_Capacity)+1;
		}

		FORCEINLINE T* InternalAllocateElements(SizeType inCapacity)
		{
			constexpr SizeType elementByteSize = sizeof(T);
			return reinterpret_cast<T*>(malloc(static_cast<size_t>(elementByteSize) * inCapacity));
		}

		FORCEINLINE void InternalReleaseData()
		{
			if (m_pData)
			{
				free(m_pData);
			}
		}

		FORCEINLINE void InternalAllocData(SizeType inCapacity)
		{
			if (inCapacity > m_Capacity)
			{
				InternalReleaseData();

				m_pData = InternalAllocateElements(inCapacity);
				m_Capacity = inCapacity;
			}
		}

		FORCEINLINE void InternalRealloc(SizeType inCapacity)
		{
			T* tempData = InternalAllocateElements(inCapacity);
			InternalMoveEmplace(m_pData, m_pData + m_Size, tempData);
			InternalDestructRange(m_pData, m_pData + m_Size);
			InternalReleaseData();

			m_pData = tempData;
			m_Capacity = inCapacity;
		}

		FORCEINLINE void InternalEmplaceRealloc(SizeType inCapacity, T* EmplacePos, SizeType count)
		{
			VALIDATE(inCapacity >= m_Size + count);

			const SizeType index = InternalIndex(EmplacePos);
			T* tempData = InternalAllocateElements(inCapacity);
			InternalMoveEmplace(m_pData, EmplacePos, tempData);
			if (EmplacePos != m_pData + m_Size)
			{
				InternalMoveEmplace(EmplacePos, m_pData + m_Size, tempData + index + count);
			}

			InternalDestructRange(m_pData, m_pData + m_Size);
			InternalReleaseData();

			m_pData = tempData;
			m_Capacity = inCapacity;
		}

		// Construct
		FORCEINLINE void InternalConstruct(SizeType size)
		{
			if (size > 0)
			{
				InternalAllocData(size);
				m_Size = size;
				InternalDefaultConstructRange(m_pData, m_pData + m_Size);
			}
		}

		FORCEINLINE void InternalConstruct(SizeType size, const T& value)
		{
			if (size > 0)
			{
				InternalAllocData(size);
				InternalCopyEmplace(size, value, m_pData);
				m_Size = size;
			}
		}

		template<typename TInputIt>
		FORCEINLINE void InternalConstruct(TInputIt begin, TInputIt end)
		{
			const SizeType distance = InternalDistance(begin, end);
			if (distance > 0)
			{
				InternalAllocData(distance);
				InternalCopyEmplace(begin, end, m_pData);
				m_Size = distance;
			}
		}

		FORCEINLINE void InternalMove(TArray&& other)
		{
			m_pData = other.m_pData;
			m_Size = other.m_Size;
			m_Capacity = other.m_Capacity;

			other.m_pData = nullptr;
			other.m_Size = 0;
			other.m_Capacity = 0;
		}

		// Emplace
		template<typename TInputIt>
		FORCEINLINE void InternalCopyEmplace(TInputIt begin, TInputIt end, T* dest)
		{
			// This function assumes that there is no overlap
			if constexpr (std::is_trivially_copy_constructible<T>())
			{
				const SizeType count = InternalDistance(begin, end);
				const SizeType cpySize = count * sizeof(T);
				memcpy(dest, InternalUnwrapConst(begin), cpySize);
			}
			else
			{
				while (begin != end)
				{
					new(reinterpret_cast<void*>(dest)) T(*begin);
					begin++;
					dest++;
				}
			}
		}

		FORCEINLINE void InternalCopyEmplace(SizeType size, const T& value, T* dest)
		{
			T* ItEnd = dest + size;
			while (dest != ItEnd)
			{
				new(reinterpret_cast<void*>(dest)) T(value);
				dest++;
			}
		}

		FORCEINLINE void InternalMoveEmplace(T* begin, T* end, T* dest)
		{
			// This function assumes that there is no overlap
			if constexpr (std::is_trivially_move_constructible<T>())
			{
				const SizeType count = InternalDistance(begin, end);
				const SizeType cpySize = count * sizeof(T);
				memcpy(dest, begin, cpySize);
			}
			else
			{
				while (begin != end)
				{
					new(reinterpret_cast<void*>(dest)) T(Move(*begin));
					begin++;
					dest++;
				}
			}
		}

		FORCEINLINE void InternalMemmoveBackwards(T* begin, T* end, T* dest)
		{
			VALIDATE(begin <= end);
			if (begin == end)
			{
				return;
			}

			VALIDATE(end <= m_pData + m_Capacity);

			// Move each object in the range to the destination
			const SizeType count = InternalDistance(begin, end);
			if constexpr (std::is_trivially_move_assignable<T>())
			{
				const SizeType cpySize = count * sizeof(T);
				memmove(dest, begin, cpySize); // Assumes that data can overlap
			}
			else
			{
				while (begin != end)
				{
					if constexpr (std::is_move_assignable<T>())
					{
						(*dest) = Move(*begin);
					}
					else if constexpr (std::is_copy_assignable<T>())
					{
						(*dest) = (*begin);
					}

					dest++;
					begin++;
				}
			}
		}

		FORCEINLINE void InternalMemmoveForward(T* begin, T* end, T* dest)
		{
			// Move each object in the range to the destination, starts in the "End" and moves forward
			const SizeType count = InternalDistance(begin, end);
			if constexpr (std::is_trivially_move_assignable<T>())
			{
				if (count > 0)
				{
					const SizeType cpySize = count * sizeof(T);
					const SizeType offsetSize = (count - 1) * sizeof(T);
					memmove(reinterpret_cast<char*>(dest) - offsetSize, begin, cpySize);
				}
			}
			else
			{
				while (end != begin)
				{
					end--;
					if constexpr (std::is_move_assignable<T>())
					{
						(*dest) = Move(*end);
					}
					else if constexpr (std::is_copy_assignable<T>())
					{
						(*dest) = (*end);
					}
					dest--;
				}
			}
		}

		FORCEINLINE void InternalDestruct(const T* pos)
		{
			// Calls the destructor (If it needs to be called)
			if constexpr (std::is_trivially_destructible<T>() == false)
			{
				(*pos).~T();
			}
		}

		FORCEINLINE void InternalDestructRange(const T* begin, const T* end)
		{
			VALIDATE(begin <= end);

			// Calls the destructor for every object in the range (If it needs to be called)
			if constexpr (std::is_trivially_destructible<T>() == false)
			{
				while (begin != end)
				{
					InternalDestruct(begin);
					begin++;
				}
			}
		}

		FORCEINLINE void InternalDefaultConstructRange(T* begin, T* end)
		{
			VALIDATE(begin <= end);

			// Calls the default constructor for every object in the range (If it can be called)
			if constexpr (std::is_default_constructible<T>())
			{
				while (begin != end)
				{
					new(reinterpret_cast<void*>(begin)) T();
					begin++;
				}
			}
		}

	private:
		T* m_pData;
		SizeType m_Size;
		SizeType m_Capacity;
	};
}