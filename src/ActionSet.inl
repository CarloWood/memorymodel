void ActionSet::add(Action const& action) { m_actions.set(action.id()); }
void ActionSet::remove(Action const& action) { m_actions.reset(action.id()); }
bool ActionSet::includes(Action const& action) const { return m_actions.test(action.id()); }
