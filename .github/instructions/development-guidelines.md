# Development Guidelines

## Code Simplicity and Best Practices

### Overview
This document outlines best practices for contributing to the Eclipsa Audio Plugin project. These guidelines are based on code review feedback and aim to maintain code quality, readability, and maintainability.

### Key Principle: Prefer Simple Solutions

**Always choose the simplest approach that solves the problem effectively.**

#### Before Implementing Complex Solutions:

1. **Check existing patterns** - Look for similar functionality already implemented in the codebase
2. **Use existing infrastructure** - Leverage frameworks and patterns already in place
3. **Consider multiple approaches** - Evaluate at least 2-3 different implementation strategies
4. **Choose the easiest and most maintainable solution**

### Specific Guidelines

#### 1. State Management

**❌ Avoid:** Complex thread synchronization and manual state tracking
```cpp
// Don't create complex locking mechanisms
juce::CriticalSection trackPropertiesLock_;
juce::MessageManager::callAsync([this]() { /* complex callback */ });
```

**✅ Prefer:** Use existing JUCE ValueTree infrastructure
```cpp
// Use ValueTree for state management - it's thread-safe and observable
audioProcessorValueTreeState.getParameterAsValue("parameterID");
```

#### 2. Communication Patterns

**❌ Avoid:** Manual callback systems and complex notification mechanisms

**✅ Prefer:** 
- JUCE's built-in parameter system
- ValueTree listeners
- Existing plugin parameter architecture

#### 3. Thread Safety

**❌ Avoid:** Custom locking mechanisms unless absolutely necessary

**✅ Prefer:**
- JUCE's thread-safe classes (ValueTree, AudioParameterFloat, etc.)
- MessageManager for UI updates
- Existing plugin threading patterns

#### 4. Code Review Checklist

Before submitting a PR, ask yourself:

- [ ] **Is this the simplest solution?** - Could this be implemented more simply?
- [ ] **Does this use existing patterns?** - Am I reinventing something that already exists?
- [ ] **Is this maintainable?** - Will other developers easily understand this code?
- [ ] **Have I considered alternatives?** - Did I explore multiple implementation approaches?
- [ ] **Does this follow project conventions?** - Am I consistent with the existing codebase?

### Implementation Process

1. **Research existing solutions** in the codebase first
2. **Prototype the simplest approach** that could work
3. **Compare with more complex alternatives** - document why simple won't work if needed
4. **Implement the chosen solution** with clear, readable code
5. **Test thoroughly** but keep test code simple too
6. **Document any non-obvious design decisions**

### When Complex Solutions Are Acceptable

Complex implementations may be justified when:
- Simple solutions have been tried and proven insufficient
- Performance requirements demand optimization
- External API constraints require specific approaches
- **Always document the rationale** for choosing complexity

### Examples from Recent Feedback

**Issue:** Complex track name synchronization with manual locking and callbacks

**Feedback:** "Would it be possible to make this less complex?"

**Solution:** Use existing ValueTree parameter system instead of custom synchronization

### Resources

- [JUCE Documentation](https://docs.juce.com/)
- [Project Contributing Guidelines](../../CONTRIBUTING.md)
- [Plugin Architecture Overview](../../README.md)

### Questions?

If you're unsure about the best approach:
1. Check existing similar implementations in the codebase
2. Ask in the PR discussion before implementing
3. Prototype multiple approaches if time allows
4. When in doubt, choose simplicity

---

*Remember: Code is read more often than it's written. Optimize for readability and maintainability.*
