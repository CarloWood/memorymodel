#pragma once

#include <iosfwd>
#include <cstdint>

struct EdgeTypePod {
  uint32_t mask;
};

EdgeTypePod const edge_sb    = { 0x1 };        // Sequenced-Before.
EdgeTypePod const edge_asw   = { 0x2 };        // Additional-Synchronizes-With.
EdgeTypePod const edge_dd    = { 0x4 };        // Data-Dependency.
EdgeTypePod const edge_cd    = { 0x8 };        // Control-Dependency.
EdgeTypePod const edge_opsem = { edge_sb.mask | edge_asw.mask | edge_dd.mask | edge_cd.mask };
// Next we have several relations that are existentially quantified: for each choice of control-flow paths = { 0x };
// the program enumerates all possible alternatives of the following:
EdgeTypePod const edge_rf    = { 0x10 };       // The Reads-From relation, from writes to all the reads that read from them.
EdgeTypePod const edge_tot   = { 0x20 };       // The TOTal order of the tot model.
EdgeTypePod const edge_mo    = { 0x40 };       // The Modification Order (or coherence order) over writes to atomic locations, total per location.
EdgeTypePod const edge_sc    = { 0x80 };       // A total order over the Sequentially Consistent atomic actions.
EdgeTypePod const edge_lo    = { 0x100 };      // The Lock Order.
EdgeTypePod const edge_witness = { edge_rf.mask |edge_mo.mask |edge_sc.mask };
// Finally there are derived relations = { 0x }; calculated by the model from the relations above:
EdgeTypePod const edge_hb    = { 0x200 };      // Happens-before.
EdgeTypePod const edge_vse   = { 0x400 };      // Visible side effects.
EdgeTypePod const edge_vsses = { 0x800 };      // Visible sequences of side effects.
EdgeTypePod const edge_ithb  = { 0x1000 };     // Inter-thread happens-before.
EdgeTypePod const edge_dob   = { 0x2000 };     // Dependency-ordered-before.
EdgeTypePod const edge_cad   = { 0x4000 };     // Carries-a-dependency-to.
EdgeTypePod const edge_sw    = { 0x8000 };     // Synchronizes-with.
EdgeTypePod const edge_hrs   = { 0x10000 };    // Hypothetical release sequence.
EdgeTypePod const edge_rs    = { 0x20000 };    // Release sequence.
EdgeTypePod const edge_dr    = { 0x40000 };    // Inter-thread data races, unrelated by hb.
EdgeTypePod const edge_ur    = { 0x80000 };    // Intra-thread unsequenced races = 0x; unrelated by sb.
EdgeTypePod const edge_undirected = { edge_dr.mask | edge_ur.mask };

int const edge_bits = 20;

struct EdgeType : public EdgeTypePod
{
  explicit EdgeType(uint32_t m) { mask = m; }
  EdgeType(EdgeTypePod pod) { mask = pod.mask; }
  friend bool operator&(EdgeTypePod edge_type1, EdgeTypePod edge_type2) { return edge_type1.mask & edge_type2.mask; }
  bool is_opsem() const { return mask & edge_opsem.mask; }
  bool is_directed() const { return !(mask & edge_undirected.mask); }
  bool operator==(EdgeTypePod edge_type) const { return mask == edge_type.mask; }
  char const* color() const;
  char const* name() const;
};

std::ostream& operator<<(std::ostream& os, EdgeType edge_type);
