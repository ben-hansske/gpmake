#pragma once

/**
 *  @file core/ring_queue.hpp
 *  @author Robert Oleynik
 *
 *  @brief Implementation of pointer based ring queue
 *
 *  changes:
 *  23.03.2020: 
 *  	- Initial ptr_queue setup containing push,pop and is_empty.
 */

#include<thread>
#include<mutex>
#include<limits>

namespace detail {
	/**
	 *	@class queue_helper_t
	 *
	 *	@brief Helper type for constructing elements
	 *
	 *	@tparam T Data type of the queue. T has to be moveable and move constructable.
	 */
	template<typename T>
	struct queue_helper_t {
		template<typename... Targs>
		void construct(Targs&&... args) {
			new (&data) T(std::forward<Targs>(args)...);
		}

		void destruct() {
			reinterpret_cast<T*>(&data)->~T();
		}

		T& ref() {
			return *reinterpret_cast<T*>(&data);
		}

		std::aligned_storage_t<sizeof(T),alignof(T)> data;
	};

	/**
	 *	@class ext_queue_capacity_bit
	 */
	struct ext_queue_capacity_bit {
		size_t queue_is_empty : 1;
		size_t capacity : sizeof(size_t)-1;
	};
}

/**
 *  @class queue
 *
 *  @brief Ring queue implementation.
 *
 *  @tparam T Data type of the queue. T has to be moveable and move constructable.
 */
template<typename T>
class queue {
	using it = detail::queue_helper_t<T>*;

public:
	/**
	 *	@brief Allocate the queue with spezified count of elements.
	 *
	 *	@param size Count of allocated elements.
	 */
	queue(size_t capacity) {
		m_data = new detail::queue_helper_t<T>[capacity]();
		m_capacity.queue_is_empty = 1;
		m_capacity.capacity = capacity;
		m_beg = m_data;
		m_end = m_data;
	}
	/**
	 *	@brief Destructs the queued data.
	 */
	~queue() {
		for(size_t i = 0; i < m_capacity.capacity; i++) {
			reinterpret_cast<T*>(m_data+i)->~T();
		}
	}

	/**
	 *	@brief Constructs T with given arguments and push it into the queue.
	 *
	 *	@param args Constructor arguments of T.
	 */
	template<typename... Targs>
	void push(Targs&&... args) {
		if(m_end == m_beg && m_capacity.queue_is_empty) {
			auto new_data = new detail::queue_helper_t<T>[m_capacity.capacity*2]();
			// Copy queue begin
			auto new_data_it = new_data;
			auto old_data_it = m_data;
			while(old_data_it != m_end) {
				new_data_it->construct(std::move(old_data_it->ref()));
				++old_data_it;
				++new_data_it;
			}
			// Copy queue end		
			auto new_end_it = new_data_it + m_capacity.capacity*2;
			auto old_end_it = m_data + m_capacity.capacity;
			do {
				--new_end_it;
				--old_end_it;
				new_end_it->construct(std::move(old_end_it->ref()));
			} while(old_end_it != m_beg);
			
			for(size_t i = 0; i < m_capacity.capacity; i++) {
				reinterpret_cast<T*>(m_data+i)->~T();
			}

			m_data = new_data;
			m_capacity.capacity = 2*m_capacity.capacity + 1;

			m_beg = new_end_it;
			m_end = new_data_it;
		}
		m_end->construct(std::forward<Targs>(args)...);
		++m_end;
		if(m_end == m_data + m_capacity.capacity) {
			m_end = m_data;
		}
		// Required to distinguish an empty queue from a full queue (m_beg == m_end)
		m_capacity.capacity = m_capacity.capacity;
		m_capacity.queue_is_empty = 0;
	}

	/**
	 *	@brief Move a pointer from the queue.
	 *
	 *	@return Returns moved pointer. Pointer can not been discarded.
	 */
	[[nodiscard]] auto pop() -> T {
		if(is_empty()) {
			throw std::runtime_error("Can not pop element from an empty queue");
		}
		auto res = std::move(m_beg->ref());
		++m_beg;
		if(m_beg == m_data+m_capacity.capacity) {
			m_beg = m_data;
		}
		if(m_end == m_beg) {
			// Required to distinguish an empty queue from a full queue (m_beg == m_end)
			m_capacity.queue_is_empty = 1;
		}
		return res;
	}

	/**
	 *	@brief Returns true if queue contains no element.
	 */
	bool is_empty() {
		return m_capacity.queue_is_empty;
	}

private:
	it								m_beg,m_end;
	it					 			m_data;
	// if first bit is 1 then the queue is empty
	detail::ext_queue_capacity_bit	m_capacity;
	std::mutex						m_mtx;

};

