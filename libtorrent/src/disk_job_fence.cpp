/*

Copyright (c) 2003, Daniel Wallin
Copyright (c) 2016-2020, Arvid Norberg
Copyright (c) 2017, 2020, Alden Torres
All rights reserved.

You may use, distribute and modify this code under the terms of the BSD license,
see LICENSE file.
*/


#include "libtorrent/aux_/disk_job_fence.hpp"
#include "libtorrent/aux_/mmap_disk_job.hpp"
#include "libtorrent/performance_counters.hpp"

#define DEBUG_STORAGE 0

#if DEBUG_STORAGE
#define DLOG(...) std::fprintf(__VA_ARGS__)
#else
#define DLOG(...) do {} while (false)
#endif

namespace libtorrent {
namespace aux {

	int disk_job_fence::job_complete(mmap_disk_job* j, tailqueue<mmap_disk_job>& jobs)
	{
		std::lock_guard<std::mutex> l(m_mutex);

		TORRENT_ASSERT(j->flags & mmap_disk_job::in_progress);
		j->flags &= ~mmap_disk_job::in_progress;

		TORRENT_ASSERT(m_outstanding_jobs > 0);
		--m_outstanding_jobs;
		if (j->flags & mmap_disk_job::fence)
		{
			// a fence job just completed. Make sure the fence logic
			// works by asserting m_outstanding_jobs is in fact 0 now
			TORRENT_ASSERT(m_outstanding_jobs == 0);

			// the fence can now be lowered
			--m_has_fence;

			// now we need to post all jobs that have been queued up
			// while this fence was up. However, if there's another fence
			// in the queue, stop there and raise the fence again
			int ret = 0;
			while (!m_blocked_jobs.empty())
			{
				mmap_disk_job *bj = m_blocked_jobs.pop_front();
				if (bj->flags & mmap_disk_job::fence)
				{
					// we encountered another fence. We cannot post anymore
					// jobs from the blocked jobs queue. We have to go back
					// into a raised fence mode and wait for all current jobs
					// to complete. The exception is that if there are no jobs
					// executing currently, we should add the fence job.
					if (m_outstanding_jobs == 0 && jobs.empty())
					{
						TORRENT_ASSERT(!(bj->flags & mmap_disk_job::in_progress));
						bj->flags |= mmap_disk_job::in_progress;
						++m_outstanding_jobs;
						++ret;
#if TORRENT_USE_ASSERTS
						TORRENT_ASSERT(bj->blocked);
						bj->blocked = false;
#endif
						jobs.push_back(bj);
					}
					else
					{
						// put the fence job back in the blocked queue
						m_blocked_jobs.push_front(bj);
					}
					return ret;
				}
				TORRENT_ASSERT(!(bj->flags & mmap_disk_job::in_progress));
				bj->flags |= mmap_disk_job::in_progress;

				++m_outstanding_jobs;
				++ret;
#if TORRENT_USE_ASSERTS
				TORRENT_ASSERT(bj->blocked);
				bj->blocked = false;
#endif
				jobs.push_back(bj);
			}
			return ret;
		}

		// there are still outstanding jobs, even if we have a
		// fence, it's not time to lower it yet
		// also, if we don't have a fence, we're done
		if (m_outstanding_jobs > 0 || m_has_fence == 0) return 0;

		// there's a fence raised, and no outstanding operations.
		// it means we can execute the fence job right now.
		TORRENT_ASSERT(m_blocked_jobs.size() > 0);

		// this is the fence job
		mmap_disk_job *bj = m_blocked_jobs.pop_front();
		TORRENT_ASSERT(bj->flags & mmap_disk_job::fence);

		TORRENT_ASSERT(!(bj->flags & mmap_disk_job::in_progress));
		bj->flags |= mmap_disk_job::in_progress;

		++m_outstanding_jobs;
#if TORRENT_USE_ASSERTS
		TORRENT_ASSERT(bj->blocked);
		bj->blocked = false;
#endif
		// prioritize fence jobs since they're blocking other jobs
		jobs.push_front(bj);
		return 1;
	}

	bool disk_job_fence::is_blocked(mmap_disk_job* j)
	{
		std::lock_guard<std::mutex> l(m_mutex);
		DLOG(stderr, "[%p] is_blocked: fence: %d num_outstanding: %d\n"
			, static_cast<void*>(this), m_has_fence, int(m_outstanding_jobs));

		// if this is the job that raised the fence, don't block it
		// ignore fence can only ignore one fence. If there are several,
		// this job still needs to get queued up
		if (m_has_fence == 0)
		{
			TORRENT_ASSERT(!(j->flags & mmap_disk_job::in_progress));
			j->flags |= mmap_disk_job::in_progress;
			++m_outstanding_jobs;
			return false;
		}

		m_blocked_jobs.push_back(j);

#if TORRENT_USE_ASSERTS
		TORRENT_ASSERT(j->blocked == false);
		j->blocked = true;
#endif

		return true;
	}

	bool disk_job_fence::has_fence() const
	{
		std::lock_guard<std::mutex> l(m_mutex);
		return m_has_fence != 0;
	}

	int disk_job_fence::num_blocked() const
	{
		std::lock_guard<std::mutex> l(m_mutex);
		return m_blocked_jobs.size();
	}

	// j is the fence job. It must have exclusive access to the storage
	int disk_job_fence::raise_fence(mmap_disk_job* j, counters& cnt)
	{
		TORRENT_ASSERT(!(j->flags & mmap_disk_job::in_progress));
		TORRENT_ASSERT(!(j->flags & mmap_disk_job::fence));
		j->flags |= mmap_disk_job::fence;

		std::lock_guard<std::mutex> l(m_mutex);

		DLOG(stderr, "[%p] raise_fence: fence: %d num_outstanding: %d\n"
			, static_cast<void*>(this), m_has_fence, int(m_outstanding_jobs));

		if (m_has_fence == 0 && m_outstanding_jobs == 0)
		{
			++m_has_fence;
			DLOG(stderr, "[%p] raise_fence: need posting\n"
				, static_cast<void*>(this));

			// the job j is expected to be put on the job queue
			// after this, without being passed through is_blocked()
			// that's why we're accounting for it here

			j->flags |= mmap_disk_job::in_progress;
			++m_outstanding_jobs;
			return fence_post_fence;
		}

		++m_has_fence;
#if TORRENT_USE_ASSERTS
		TORRENT_ASSERT(j->blocked == false);
		j->blocked = true;
#endif
		m_blocked_jobs.push_back(j);
		cnt.inc_stats_counter(counters::blocked_disk_jobs);

		return fence_post_none;
	}

}
}
