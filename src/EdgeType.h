#pragma once

#include <iosfwd>
#include <cstdint>

enum EdgeType {
  edge_sb,
  edge_asw,
  edge_dd,
  edge_cd,
  edge_rf,
  edge_tot,
  edge_mo,
  edge_sc,
  edge_lo,
  edge_hb,
  edge_vse,
  edge_vsses,
  edge_ithb,
  edge_dob,
  edge_cad,
  edge_sw,
  edge_hrs,
  edge_rs,
  edge_dr,
  edge_ur,
  edge_bits     // Number of bits in the mask below.
};

using edge_mask_type = uint32_t;

constexpr edge_mask_type to_mask(EdgeType edge_type)
{
  return edge_mask_type{1} << edge_type;
}

struct EdgeMaskTypePod {
  edge_mask_type mask;
};

EdgeMaskTypePod constexpr edge_mask_sb    = { to_mask(edge_sb) };       // Sequenced-Before.
EdgeMaskTypePod constexpr edge_mask_asw   = { to_mask(edge_asw) };      // Additional-Synchronizes-With.
EdgeMaskTypePod constexpr edge_mask_dd    = { to_mask(edge_dd) };       // Data-Dependency.
EdgeMaskTypePod constexpr edge_mask_cd    = { to_mask(edge_cd) };       // Control-Dependency.
EdgeMaskTypePod constexpr edge_mask_sbw   = { edge_mask_sb.mask | edge_mask_asw.mask };
EdgeMaskTypePod constexpr edge_mask_opsem = { edge_mask_sb.mask | edge_mask_asw.mask | edge_mask_dd.mask | edge_mask_cd.mask };
// Next we have several relations that are existentially quantified: for each choice of control-flow paths;
// the program enumerates all possible alternatives of the following:
EdgeMaskTypePod constexpr edge_mask_rf    = { to_mask(edge_rf) };       // The Reads-From relation, from writes to all the reads that read from them.
EdgeMaskTypePod constexpr edge_mask_tot   = { to_mask(edge_tot) };      // The TOTal order of the tot model.
EdgeMaskTypePod constexpr edge_mask_mo    = { to_mask(edge_mo) };       // The Modification Order (or coherence order) over writes to atomic locations, total per location.
EdgeMaskTypePod constexpr edge_mask_sc    = { to_mask(edge_sc) };       // A total order over the Sequentially Consistent atomic actions.
EdgeMaskTypePod constexpr edge_mask_lo    = { to_mask(edge_lo) };       // The Lock Order.
EdgeMaskTypePod constexpr edge_mask_witness = { edge_mask_rf.mask |edge_mask_mo.mask |edge_mask_sc.mask };
// Finally there are derived relations; calculated by the model from the relations above:
EdgeMaskTypePod constexpr edge_mask_hb    = { to_mask(edge_hb) };       // Happens-before.
EdgeMaskTypePod constexpr edge_mask_vse   = { to_mask(edge_vse) };      // Visible side effects.
EdgeMaskTypePod constexpr edge_mask_vsses = { to_mask(edge_vsses) };    // Visible sequences of side effects.
EdgeMaskTypePod constexpr edge_mask_ithb  = { to_mask(edge_ithb) };     // Inter-thread happens-before.
EdgeMaskTypePod constexpr edge_mask_dob   = { to_mask(edge_dob) };      // Dependency-ordered-before.
EdgeMaskTypePod constexpr edge_mask_cad   = { to_mask(edge_cad) };      // Carries-a-dependency-to.
EdgeMaskTypePod constexpr edge_mask_sw    = { to_mask(edge_sw) };       // Synchronizes-with.
EdgeMaskTypePod constexpr edge_mask_hrs   = { to_mask(edge_hrs) };      // Hypothetical release sequence.
EdgeMaskTypePod constexpr edge_mask_rs    = { to_mask(edge_rs) };       // Release sequence.
EdgeMaskTypePod constexpr edge_mask_dr    = { to_mask(edge_dr) };       // Inter-thread data races, unrelated by hb.
EdgeMaskTypePod constexpr edge_mask_ur    = { to_mask(edge_ur) };       // Intra-thread unsequenced races; unrelated by sb.
EdgeMaskTypePod constexpr edge_mask_undirected = { edge_mask_dr.mask | edge_mask_ur.mask };

struct EdgeMaskType : public EdgeMaskTypePod
{
  explicit EdgeMaskType(EdgeType edge_type) { mask = to_mask(edge_type); }
  EdgeMaskType(EdgeMaskTypePod pod) { mask = pod.mask; }
  friend bool operator&(EdgeMaskTypePod edge_mask_type1, EdgeMaskTypePod edge_mask_type2) { return edge_mask_type1.mask & edge_mask_type2.mask; }
  friend bool operator&(EdgeType edge_type, EdgeMaskTypePod edge_mask_type) { return to_mask(edge_type) & edge_mask_type.mask; }
  friend bool operator&(EdgeMaskTypePod edge_mask_type, EdgeType edge_type) { return to_mask(edge_type) & edge_mask_type.mask; }
  bool is_opsem() const { return mask & edge_mask_opsem.mask; }
  bool is_directed() const { return !(mask & edge_mask_undirected.mask); }
  bool operator==(EdgeMaskTypePod edge_mask_type) const { return mask == edge_mask_type.mask; }
};

char const* edge_color(EdgeType edge_type);
char const* edge_name(EdgeType edge_type);

std::ostream& operator<<(std::ostream& os, EdgeType edge_type);
std::ostream& operator<<(std::ostream& os, EdgeMaskType edge_mask_type);
