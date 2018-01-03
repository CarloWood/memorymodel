#pragma once

#include <iosfwd>
#include "Thread.h"
#include "TopologicalOrderedActions.h"
#include "RFLocationOrderedSubgraphs.h"
#include "boolean-expression/BooleanExpression.h"

class ReadFromGraph;
class Propagator;

enum property_type {
  causal_loop,
  reads_from
};

enum property_merge_type {
  merge_none,
  merge_simple,
  merge_causal_loop
};

class Property
{
 private:
  property_type m_type;                 // The property type.
  SequenceNumber m_end_point;           // causal_loop: the first node that was found that was already visited while following this path.
                                        // reads_from : the node that is being read.
  boolean::Expression m_path_condition; // The condition under which this property will exist.

  // Only valid for causal_loop:
  SequenceNumber m_rs_end;              // The head of the Release Sequence.
  bool m_not_synced_yet;                // Set to true after following a rf edge from a Read-acq to a non-rel Write, until we hit the rs-tail.
  bool m_broken_release_sequence;       // Set to true if there is a non-release write between the rs-tail and the rs-head.

  // Only valid when m_not_synced_yet:
  RFLocation m_not_synced_location;     // The memory location that isn't synced yet.
  Thread::id_type m_not_synced_thread;  // The thread that did a relaxed write to an unsynced memory location, if any (-1 if none or not relevant).

  // Only valid for reads_from:
  bool m_hidden;                        // Set to true when a write to the corresponding memory location will happen before reaching m_end_point.

 public:
  Property(property_type type, SequenceNumber end_point, SequenceNumber rs_end, boolean::Expression const& path_condition) :
      m_type(type), m_end_point(end_point), m_path_condition(path_condition.copy()),
      m_rs_end(rs_end), m_not_synced_yet(false), m_broken_release_sequence(false), m_not_synced_thread(-1), m_hidden(false) { }

  Property(Property const& property, boolean::Expression&& path_condition) :
      m_type(property.m_type),
      m_end_point(property.m_end_point),
      m_path_condition(std::move(path_condition)),
      m_rs_end(property.m_rs_end),
      m_not_synced_yet(property.m_not_synced_yet),
      m_broken_release_sequence(property.m_broken_release_sequence),
      m_not_synced_location(property.m_not_synced_location),
      m_not_synced_thread(property.m_not_synced_thread),
      m_hidden(property.m_hidden) { }

  // Return true when this property is relevant to be copied from child to parent node.
  bool is_relevant(ReadFromGraph const& read_from_graph) const;

  // Accessors.
  property_type type() const { return m_type; }
  SequenceNumber end_point() const { return m_end_point; }
  boolean::Expression const& path_condition() const { return m_path_condition; }

  bool invalidates_graph(ReadFromGraph const* read_from_graph) const;
  bool convert(Propagator const& propagator);
  void merge_into(std::vector<Property>& map);
  friend property_merge_type need_merging(Property const& p1, Property const& p2);

  friend std::ostream& operator<<(std::ostream& os, Property const& property);
};
