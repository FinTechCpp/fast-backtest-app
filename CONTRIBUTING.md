# Contributing to Fast Backtest App

Thank you for your interest in contributing to Fast Backtest App! We welcome contributions from the community and are grateful for your support.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [How Can I Contribute?](#how-can-i-contribute)
  - [Reporting Bugs](#reporting-bugs)
  - [Suggesting Features](#suggesting-features)
  - [Contributing Code](#contributing-code)
  - [Improving Documentation](#improving-documentation)
- [Development Setup](#development-setup)
- [Coding Standards](#coding-standards)
- [Commit Guidelines](#commit-guidelines)
- [Pull Request Process](#pull-request-process)
- [Testing](#testing)
- [Project Structure](#project-structure)
- [Community](#community)

---

## Code of Conduct

This project adheres to a code of conduct that we expect all contributors to follow. Please be respectful, constructive, and collaborative in all interactions.

### Our Standards

- **Be respectful**: Treat everyone with respect and kindness
- **Be constructive**: Provide helpful feedback and accept criticism gracefully
- **Be collaborative**: Work together to achieve the best outcomes
- **Be inclusive**: Welcome newcomers and help them get started
- **Be professional**: Keep discussions focused on the project

---

## How Can I Contribute?

### Reporting Bugs

Bugs are tracked as [GitHub issues](https://github.com/FinTechCpp/fast-backtest-app/issues). Before creating a bug report, please check if the issue has already been reported.

#### Before Submitting a Bug Report

- **Check the documentation** to see if the behavior is expected
- **Search existing issues** to avoid duplicates
- **Try the latest version** to see if the bug has been fixed
- **Collect relevant information** (OS, Qt version, compiler version, logs)

#### How to Submit a Good Bug Report

A good bug report should include:

1. **Clear title**: Summarize the issue in one line
2. **Environment details**:
   - Operating System (Windows 11, Ubuntu 22.04, macOS 13, etc.)
   - Qt version (6.8.3)
   - CMake version (4.0.3)
   - Compiler version (GCC 13, MSVC 2019, etc.)
   - Application version (from Help → About)
3. **Steps to reproduce**:
   - Exact steps to trigger the bug
   - Input data (CSV files, strategy configuration)
   - Expected behavior vs actual behavior
4. **Logs and screenshots**:
   - Relevant log files from `logs/backtestApp/`
   - Screenshots showing the issue
   - Error messages (full text)
5. **Additional context**: Any other relevant information

#### Example Bug Report

```markdown
**Title**: Application crashes when loading CSV with missing volume column

**Environment**:
- OS: Windows 11 22H2
- Qt: 6.8.3
- CMake: 4.0.3
- Compiler: MSVC 2019 (19.29)
- App version: 1.0.0.31

**Steps to Reproduce**:
1. Launch the application
2. Go to Data → Import CSV
3. Select a CSV file without volume column
4. Click "Import"

**Expected Behavior**: 
The application should import the CSV and use 0 for missing volume values.

**Actual Behavior**: 
The application crashes with error: "Index out of range"

**Logs**:
```
[2025-10-31 10:23:45] [ERROR] DataLoader: Column 'volume' not found
[2025-10-31 10:23:45] [CRITICAL] Application crashed: std::out_of_range
```

**Additional Context**:
CSV file format:
```
timestamp,open,high,low,close
2024-01-01T09:30:00,100.5,101.2,100.1,101.0
```
```

### Suggesting Features

We welcome feature suggestions! Before submitting a feature request, please:

1. **Check existing issues** to see if it's already proposed
2. **Review the roadmap** in the README to see if it's planned
3. **Consider the scope**: Does it fit the project's goals?

#### How to Submit a Good Feature Request

Include:

1. **Clear title**: What feature do you want?
2. **Problem statement**: What problem does it solve?
3. **Proposed solution**: How should it work?
4. **Alternatives considered**: Other ways to solve the problem
5. **Use cases**: Real-world examples of how you'd use it
6. **Implementation ideas**: (Optional) Technical approach

#### Example Feature Request

```markdown
**Title**: Add Monte Carlo simulation for strategy validation

**Problem**: 
Traders need to validate if their strategy results are statistically significant or just lucky.

**Proposed Solution**:
Add a Monte Carlo simulation feature that:
- Randomly shuffles trade sequences
- Runs 1000+ simulations
- Shows distribution of possible outcomes
- Calculates confidence intervals

**Use Cases**:
- Validate if a strategy with 60% win rate is statistically significant
- Estimate worst-case scenarios
- Compare multiple strategies with uncertainty bands

**Implementation Ideas**:
- New tab in results view: "Monte Carlo"
- Use C++ random number generation
- Visualize with ChartDirector histogram
- Export results to CSV
```

### Contributing Code

#### First Time Contributors

If you're new to the project:

1. Look for issues labeled `good first issue` or `help wanted`
2. Comment on the issue to let others know you're working on it
3. Ask questions if you need clarification
4. Don't hesitate to request help in the issue thread

#### Development Workflow

1. **Fork the repository**
   ```bash
   # On GitHub, click "Fork"
   git clone https://github.com/YOUR_USERNAME/fast-backtest-app.git
   cd fast-backtest-app
   ```

2. **Set up upstream remote**
   ```bash
   git remote add upstream https://github.com/FinTechCpp/fast-backtest-app.git
   ```

3. **Create a feature branch**
   ```bash
   git checkout -b feature/your-feature-name
   # or
   git checkout -b fix/your-bug-fix
   ```

4. **Initialize submodules**
   ```bash
   git submodule update --init --recursive
   ```

5. **Make your changes**
   - Follow the coding standards (see below)
   - Write tests for new features
   - Update documentation as needed

6. **Commit your changes**
   ```bash
   git add .
   git commit -m "feat: Add Monte Carlo simulation"
   ```

7. **Keep your branch updated**
   ```bash
   git fetch upstream
   git rebase upstream/main
   ```

8. **Push to your fork**
   ```bash
   git push origin feature/your-feature-name
   ```

9. **Create a Pull Request**
   - Go to GitHub and create a PR from your branch
   - Fill out the PR template
   - Link related issues

### Improving Documentation

Documentation improvements are always welcome! This includes:

- **README.md**: Installation, usage, features
- **Code comments**: Explain complex logic
- **API documentation**: Doxygen comments for classes/functions
- **Wiki pages**: Tutorials, guides, examples
- **Examples**: Sample strategies, notebooks

#### Documentation Style

- Use clear, concise language
- Include code examples where helpful
- Add screenshots for UI features
- Test all commands/code snippets
- Keep documentation in sync with code

---

## Development Setup

### Prerequisites

- **CMake** 4.0.3+
- **Qt** 6.8.3
- **C++ Compiler**:
  - Linux: GCC 13+ or Clang 15+
  - Windows: MSVC 2019+ or MinGW 11+
  - macOS: Clang 15+ (Xcode 14+)
- **Git** with LFS support

### Installation

Follow the detailed instructions in [README.md](README.md#🔧-prerequisite-installation).

### Building for Development

#### Linux/macOS

```bash
# Debug build with all warnings
./build_and_run.sh --debug

# Or manually
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_WITH_DEBUG=ON ..
make -j$(nproc)
```

#### Windows

```powershell
# Create build directory
mkdir build -Force
cd build

# Configure with Debug mode
cmake .. -G "Visual Studio 17 2022" -A x64 -DBUILD_WITH_DEBUG=ON

# Build
cmake --build . --config Debug --parallel
```

### Running Tests

```bash
# Run all tests
cd build
ctest --output-on-failure

# Run specific test suite
ctest -R BacktestEngine
```

### Debugging

#### VS Code

1. Open the project in VS Code
2. Install recommended extensions (C++, CMake Tools)
3. Press F5 to start debugging
4. Use the configurations in `.vscode/launch.json`

#### Command Line (GDB)

```bash
# Build with debug symbols
./build_and_run.sh --debug --no-run

# Run with GDB
gdb ./build/backtestApp/backtestapp
(gdb) break main
(gdb) run
```

#### Visual Studio

1. Open `build/fast-backtest-app.sln`
2. Set `backtestapp` as startup project
3. Press F5 to debug

---

## Coding Standards

### C++ Style Guide

We follow a hybrid style based on Qt conventions and modern C++ best practices.

#### General Principles

- **Clarity over cleverness**: Write readable code
- **RAII**: Use smart pointers and containers
- **Const correctness**: Mark everything const that should be
- **Avoid raw pointers**: Use `std::unique_ptr`, `std::shared_ptr`
- **Prefer algorithms**: Use STL algorithms over raw loops

#### Naming Conventions

```cpp
// Classes: PascalCase
class BacktestEngine { };
class DataLoader { };

// Functions: camelCase (Qt style)
void loadData();
void calculateMetrics();

// Member variables: m_ prefix + camelCase
class MyClass {
private:
    int m_count;
    QString m_name;
    std::unique_ptr<Data> m_data;
};

// Constants: ALL_CAPS or k prefix
const int MAX_TRADES = 10000;
constexpr double kDefaultSpread = 0.001;

// Namespaces: lowercase
namespace be { }  // backtest engine
namespace utils { }

// STL-style (in Strategy code): snake_case
void calculate_indicators();
double compute_sharpe_ratio();
```

#### Code Formatting

```cpp
// Braces: K&R style (opening brace on same line)
void myFunction() {
    if (condition) {
        doSomething();
    } else {
        doSomethingElse();
    }
}

// Indentation: 4 spaces (no tabs)
class MyClass {
public:
    void method() {
        if (condition) {
            // code
        }
    }
    
private:
    int m_member;
};

// Pointer/reference: * and & next to type
QString* pointer;
const QString& reference;

// Long function calls: align parameters
auto result = someLongFunctionName(
    parameter1,
    parameter2,
    parameter3
);
```

#### Comments

```cpp
/**
 * @brief Calculate Sharpe ratio for a strategy
 * 
 * @param returns Vector of returns
 * @param riskFreeRate Annual risk-free rate (default: 0.0)
 * @return double Sharpe ratio
 */
double calculateSharpeRatio(const std::vector<double>& returns, 
                           double riskFreeRate = 0.0);

// Use // for single-line comments
// TODO: Optimize this algorithm

/* Use block comments for 
   multi-line explanations
   when needed */
```

#### Qt-Specific

```cpp
// Use Qt types when interfacing with Qt
QString text = lineEdit->text();
QVector<double> data;
QList<Trade> trades;

// Signals and slots
class MyWidget : public QWidget {
    Q_OBJECT
    
signals:
    void dataChanged(const QString& newData);
    
private slots:
    void onButtonClicked();
    
private:
    QPushButton* m_button;
};

// Connect with new-style syntax
connect(m_button, &QPushButton::clicked,
        this, &MyWidget::onButtonClicked);
```

#### Modern C++ Features

```cpp
// Use auto when type is obvious
auto widget = new QWidget();
auto value = calculateValue();

// Range-based for loops
for (const auto& trade : trades) {
    processTrade(trade);
}

// Smart pointers
std::unique_ptr<Data> data = std::make_unique<Data>();
std::shared_ptr<Strategy> strategy = std::make_shared<Strategy>();

// Structured bindings (C++17)
auto [min, max] = std::minmax_element(vec.begin(), vec.end());

// Optional for nullable values
std::optional<double> result = findValue();
if (result.has_value()) {
    use(*result);
}
```

### Header Files

```cpp
// Use header guards
#pragma once

// Include order: own header, Qt, std, project
#include "myclass.h"
#include <QWidget>
#include <QString>
#include <vector>
#include <memory>
#include "utils/helper.h"

// Forward declarations when possible
class QTimer;
class BacktestEngine;
```

### Error Handling

```cpp
// Qt logging
qDebug() << "Debug message";
qWarning() << "Warning message";
qCritical() << "Critical error";

// Custom logger (spdlog)
logger->info("Processing {} trades", count);
logger->warn("High memory usage: {} MB", memory);
logger->error("Failed to load file: {}", filename);

// Exceptions (use sparingly in Qt code)
if (!file.open(QIODevice::ReadOnly)) {
    throw std::runtime_error("Cannot open file");
}
```

---

## Commit Guidelines

We follow the [Conventional Commits](https://www.conventionalcommits.org/) specification.

### Commit Message Format

```
<type>(<scope>): <subject>

<body>

<footer>
```

### Types

- **feat**: New feature
- **fix**: Bug fix
- **docs**: Documentation changes
- **style**: Code style changes (formatting, no logic change)
- **refactor**: Code refactoring
- **perf**: Performance improvements
- **test**: Adding or updating tests
- **build**: Build system changes
- **ci**: CI/CD changes
- **chore**: Other changes (dependencies, configs)

### Examples

```bash
# Feature
feat(chart): add zoom functionality to price charts

# Bug fix
fix(backtest): correct spread calculation for limit orders

# Documentation
docs(readme): update installation instructions for Windows

# Refactoring
refactor(engine): extract indicator calculation to separate class

# Performance
perf(backtest): optimize candle resampling algorithm

# Multiple scopes
feat(ui,chart): add interactive trade markers on charts
```

### Best Practices

- Use imperative mood ("add" not "added" or "adds")
- First line max 72 characters
- Reference issues: `Closes #123` or `Fixes #456`
- Break up large changes into smaller commits
- Each commit should be a logical unit

---

## Pull Request Process

### Before Submitting

- [ ] Code follows style guidelines
- [ ] Self-review completed
- [ ] Comments added for complex code
- [ ] Documentation updated
- [ ] Tests added/updated
- [ ] All tests pass locally
- [ ] No compiler warnings
- [ ] Commits follow commit guidelines
- [ ] Branch is up-to-date with main

### PR Template

```markdown
## Description
Brief description of changes

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Breaking change
- [ ] Documentation update

## Related Issues
Closes #123

## Changes Made
- Added Monte Carlo simulation
- Updated results view with new tab
- Added unit tests

## Testing
- [ ] Tested on Linux
- [ ] Tested on Windows
- [ ] Tested on macOS
- [ ] Manual testing completed

## Screenshots
(if applicable)

## Checklist
- [ ] Code follows style guidelines
- [ ] Documentation updated
- [ ] Tests added
- [ ] All tests pass
```

### Review Process

1. **Automated checks**: CI must pass
2. **Code review**: At least one maintainer approval
3. **Testing**: Verify on multiple platforms if possible
4. **Merge**: Maintainers will merge when ready

### After Merge

- Delete your feature branch
- Update your local main branch
- Close related issues if not auto-closed

---

## Testing

### Unit Tests

TODO

### Integration Tests

Test complete workflows:
- Load CSV → Run backtest → Verify results
- Create strategy → Save profile → Load profile
- Import data → Resample → Export

### Manual Testing

Before submitting, test:
1. **UI interactions**: Click all buttons, test all inputs
2. **Edge cases**: Empty data, invalid inputs, extreme values
3. **Performance**: Large datasets, complex strategies
4. **Platforms**: Windows, Linux, macOS (if possible)

---

## Project Structure

Understanding the codebase:

```
fast-backtest-app/
├── backtestApp/              # Qt GUI application
│   ├── include/
│   │   ├── ui/              # UI headers (views, dialogs)
│   │   └── components/      # Business logic components
│   └── src/
│       ├── main.cpp         # Application entry point
│       ├── ui/              # UI implementations
│       └── components/      # Component implementations
│
├── ThirdParty/
│   ├── fast-backtest-engine/  # Core C++ backtest engine
│   │   ├── include/
│   │   │   ├── beTypes.h       # Core types
│   │   │   ├── data.hpp        # Data structures
│   │   │   └── Managers/       # Position, candle managers
│   │   └── src/
│   │
│   └── Strategies/             # Strategy framework
│       ├── include/
│       │   ├── strategy.hpp    # Base strategy class
│       │   ├── common.h        # Common types
│       │   └── Indicators/     # Technical indicators
│       └── src/
│
├── marketData/                 # Market data CSV files
├── Notebooks/                  # Jupyter notebooks
├── logs/                       # Application logs
└── CMakeLists.txt             # Root CMake
```

### Key Components

- **App.cpp**: Main window, orchestrates UI
- **BacktestRunner**: Executes backtests in worker thread
- **DataLoader**: Loads and parses CSV data
- **Strategy**: Base class for trading strategies
- **FilterEvaluator**: Evaluates strategy filters
- **ChartView**: ChartDirector-based visualizations
- **StatsView**: Displays backtest metrics

---

## Community

### Communication Channels

- **GitHub Issues**: Bug reports, feature requests
- **GitHub Discussions**: Questions, ideas, showcase
- **Discord**: Real-time chat (coming soon)

### Getting Help

- Check [README.md](README.md) and documentation first
- Search existing issues and discussions
- Ask clear, specific questions
- Provide context and code samples
- Be patient and respectful

### Recognition

Contributors are recognized in:
- README contributors section
- Release notes
- GitHub contributor graph

---

## License

By contributing to Fast Backtest App, you agree that your contributions will be licensed under the [MIT License](LICENSE).

---

## Questions?

If you have questions about contributing, feel free to:
- Open a discussion on GitHub
- Ask in the issue thread

**Thank you for contributing to Fast Backtest App! 🚀**
