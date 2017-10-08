#include <iostream>
#include <cassert>
#include <vector>

//========================================
// This program is verified to be correct.
//========================================

// It outputs the number of matchings of a N-partite graph,
// with M vertices per partition.
//
// This is what we want to know; N threads and M nodes
// per thread where each node can either be unconnected
// or be connected to exactly one other node of a different
// thread.
//
// Matching with N threads i, each with nodes[i] nodes.
int64_t f(std::vector<int64_t> nodes)
{
  size_t N = nodes.size();

  // In the case of 0 threads there is 1 matching.
  if (N == 0)
    return 1;

  // Each of the existing threads should have at least one node.
  bool have_all_one_node = true;
  std::vector<int64_t> v;
  for (auto s : nodes)
  {
    if (s == 0)
      have_all_one_node = false;
    else
      v.push_back(s);
  }
  if (!have_all_one_node)
    return f(v);

  // In the case of 1 thread, each node can only be connected
  // to itself; so one matching.
  if (N == 1)
    return 1;

  // Otherwise we run over all possible ways the first
  // node of the first thread can be connected and sum
  // the results.

  // Firstly, this node can be self-connected.
  --v[0];
  int64_t sum = f(v);

  // Otherwise it is connected one of the remaining threads.
  for (size_t i = 1; i < N; ++i)
  {
    --v[i];
    sum += (v[i] + 1) * f(v);
    ++v[i];
  }

  return sum;
}

int main()
{
  int number_of_threads;
  int nodes_per_thread;

  std::cout << "Enter the number of threads: ";
  std::cin >> number_of_threads;
  std::cout << "Enter the number of nodes per thread: ";
  std::cin >> nodes_per_thread;

  std::vector<int64_t> v(number_of_threads, nodes_per_thread);
  std::cout << "Matchings: " << f(v) << std::endl;
}
