CXX ?= g++
CXXFLAGS ?= -std=c++23 -O2 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror

.PHONY: test sanitize

test:
	$(CXX) $(CXXFLAGS) core/tests/timer_test.cpp -o /tmp/ahc-core-timer-test
	/tmp/ahc-core-timer-test
	$(CXX) $(CXXFLAGS) core/tests/rng_test.cpp -o /tmp/ahc-core-rng-test
	/tmp/ahc-core-rng-test
	$(CXX) $(CXXFLAGS) core/tests/zobrist_test.cpp -o /tmp/ahc-core-zobrist-test
	/tmp/ahc-core-zobrist-test
	$(CXX) $(CXXFLAGS) local-search/tests/test_local_search.cpp -o /tmp/ahc-local-search-test
	/tmp/ahc-local-search-test
	$(CXX) $(CXXFLAGS) local-search/tests/best_solution_test.cpp -o /tmp/ahc-best-solution-test
	/tmp/ahc-best-solution-test
	$(CXX) $(CXXFLAGS) local-search/tests/core_integration_test.cpp -o /tmp/ahc-core-local-test
	/tmp/ahc-core-local-test
	$(CXX) $(CXXFLAGS) utilities/tests/test_utilities.cpp -o /tmp/ahc-utilities-test
	/tmp/ahc-utilities-test
	$(CXX) $(CXXFLAGS) utilities/tests/test_grid_bfs.cpp -o /tmp/ahc-grid-bfs-test
	/tmp/ahc-grid-bfs-test
	$(CXX) $(CXXFLAGS) tests/all_headers_test.cpp -o /tmp/ahc-all-headers-test
	/tmp/ahc-all-headers-test
	python3 -m unittest tools/tests/test_amalgamate.py

sanitize: export ASAN_OPTIONS=detect_leaks=0
sanitize:
	$(MAKE) test CXXFLAGS="$(CXXFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer"
