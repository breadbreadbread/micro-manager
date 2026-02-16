# File Reference Card

Quick reference to all files created for Micro-Manager Spinnaker SDK 4.x.x macOS ARM64 support.

## 📁 Main Documentation (67+ KB)

| File | Size | Purpose | When to Read |
|------|-------|---------|--------------|
| **SPINNAKER_MACOS_ARM64_PLAN.md** | 18.6 KB | Complete 12-week implementation plan | Understanding full solution |
| **SPINNAKER_QUICK_REFERENCE.md** | 9.3 KB | API reference & debugging | Development & troubleshooting |
| **SPINNAKER_IMPLEMENTATION_SUMMARY.md** | 11.7 KB | Executive summary & quick start | Getting overview |
| **SPINNAKER_ADAPTER_TEMPLATE/README.md** | 12.7 KB | Adapter user guide | Using the adapter |
| **README_FOR_YOU.md** | 10.5 KB | Your getting-started guide | **START HERE** |

## 💻 Production Code (29+ KB)

| File | Size | Language | Purpose |
|------|-------|----------|---------|
| **SPINNAKER_ADAPTER_TEMPLATE/Spinnaker4Camera.h** | 4.7 KB | C++ | Camera class definition |
| **SPINNAKER_ADAPTER_TEMPLATE/Spinnaker4Camera.cpp** | 24.6 KB | C++ | Full adapter implementation |
| **SPINNAKER_ADAPTER_TEMPLATE/Makefile.am** | 760 B | Makefile | Build configuration |

## 🛠️ Build Scripts (13+ KB)

| File | Size | Purpose | When to Use |
|------|-------|---------|-------------|
| **REBUILD_WITH_SPINNAKER.sh** | 8.5 KB | Automated build | Building Micro-Manager |
| **disable_problematic_adapters.sh** | 4.6 KB | Disable bad adapters | Multiple adapter issues |
| **test_spinnaker_env.sh** | 8.2 KB | Environment test | Verifying setup |

## 🔧 Patches (6+ KB)

| File | Size | Purpose | When to Use |
|------|-------|---------|-------------|
| **patches/disable_blueboxoptics_niji.patch** | 1.5 KB | Disable BlueboxOptics_niji | Boost lexical_cast issue |
| **patches/disable_serialmanager.patch** | 1.6 KB | Disable SerialManager | Boost Asio io_service issue |
| **patches/add_spinnaker4_support.patch** | 3.5 KB | Add Spinnaker4 to build | After Micro-Manager builds |
| **patches/README.md** | 7.8 KB | Patch instructions | Applying patches |

## 📋 Troubleshooting Guides (16+ KB)

| File | Size | Purpose |
|------|-------|---------|
| **QUICK_DISABLE_BLUEBOX.md** | 5.1 KB | Disable problematic adapters |
| **BUILD_STATUS_AND_NEXT_STEPS.md** | 10.4 KB | Build progress & next steps |

## 📊 Summary

| Category | Files | Total Size |
|----------|-------|-----------|
| **Documentation** | 6 | 75.5 KB |
| **Code** | 3 | 29.1 KB |
| **Scripts** | 3 | 13.4 KB |
| **Patches** | 4 | 14.4 KB |
| **Guides** | 2 | 15.5 KB |
| **TOTAL** | **18 files** | **147.9 KB** |

## 🚀 Quick Start Paths

### For YOU (user just starting):
```
START: README_FOR_YOU.md
THEN:  BUILD_STATUS_AND_NEXT_STEPS.md
IF:   patches/disable_blueboxoptics_niji.patch
      patches/disable_serialmanager.patch
```

### For BUILDING Micro-Manager:
```
TEST:  test_spinnaker_env.sh
AUTO:  REBUILD_WITH_SPINNAKER.sh
      or disable_problematic_adapters.sh
PATCH: patches/disable_blueboxoptics_niji.patch
      patches/disable_serialmanager.patch
```

### For DEVELOPMENT:
```
PLAN:  SPINNAKER_MACOS_ARM64_PLAN.md
CODE:  SPINNAKER_ADAPTER_TEMPLATE/*.cpp
REF:  SPINNAKER_QUICK_REFERENCE.md
```

### For USING the adapter:
```
GUIDE: SPINNAKER_ADAPTER_TEMPLATE/README.md
TEST:  Spinnaker4Camera.cpp
```

## 📚 Reading Order

### Path 1: Just want to get it working? (Fastest)
1. README_FOR_YOU.md ← Start here
2. BUILD_STATUS_AND_NEXT_STEPS.md ← Your current situation
3. QUICK_DISABLE_BLUEBOX.md ← Fix current blocker
4. patches/disable_blueboxoptics_niji.patch ← Apply it
5. patches/disable_serialmanager.patch ← Apply it too
6. disable_problematic_adapters.sh ← Or automate it

### Path 2: Want to understand the solution?
1. SPINNAKER_IMPLEMENTATION_SUMMARY.md ← Executive summary
2. SPINNAKER_MACOS_ARM64_PLAN.md ← Full plan
3. SPINNAKER_ADAPTER_TEMPLATE/README.md ← User guide
4. SPINNAKER_QUICK_REFERENCE.md ← Reference

### Path 3: Having build issues?
1. test_spinnaker_env.sh ← Test environment
2. QUICK_DISABLE_BLUEBOX.md ← Disable adapters
3. SERIALMANAGER_BOOST_ASIO_FIX.md ← SerialManager specific fix
4. BUILD_STATUS_AND_NEXT_STEPS.md ← Detailed troubleshooting
5. SPINNAKER_QUICK_REFERENCE.md → Troubleshooting section

## 🎯 Find What You Need

| Your Situation | Read This | Do This |
|---------------|------------|----------|
| Just starting | README_FOR_YOU.md | Follow quick start guide |
| Build blocked | QUICK_DISABLE_BLUEBOX.md | Disable bad adapter |
| Need to verify setup | test_spinnaker_env.sh | Run test script |
| Want to auto-build | REBUILD_WITH_SPINNAKER.sh | Run script |
| Using patches | patches/README.md | Follow patch guide |
| Understand solution | SPINNAKER_IMPLEMENTATION_SUMMARY.md | Read summary |
| Full details | SPINNAKER_MACOS_ARM64_PLAN.md | Read 12-week plan |
| Developing adapter | SPINNAKER_ADAPTER_TEMPLATE/*.cpp | Review code |
| Using adapter | SPINNAKER_ADAPTER_TEMPLATE/README.md | Follow user guide |
| API reference | SPINNAKER_QUICK_REFERENCE.md | Look up API |

## 📞 File Locations

All files in: `/home/engine/project/`

```
/home/engine/project/
├── README_FOR_YOU.md                      ← START HERE!
├── BUILD_STATUS_AND_NEXT_STEPS.md
├── QUICK_DISABLE_BLUEBOX.md
├── SERIALMANAGER_BOOST_ASIO_FIX.md       ← NEW: SerialManager fix
├── SPINNAKER_MACOS_ARM64_PLAN.md
├── SPINNAKER_QUICK_REFERENCE.md
├── SPINNAKER_IMPLEMENTATION_SUMMARY.md
├── test_spinnaker_env.sh
├── REBUILD_WITH_SPINNAKER.sh
├── disable_problematic_adapters.sh
├── patches/
│   ├── README.md
│   ├── disable_blueboxoptics_niji.patch
│   ├── disable_serialmanager.patch       ← NEW: SerialManager patch
│   └── add_spinnaker4_support.patch
└── SPINNAKER_ADAPTER_TEMPLATE/
    ├── README.md
    ├── Spinnaker4Camera.h
    ├── Spinnaker4Camera.cpp
    └── Makefile.am
```

## ✨ Success Checklist

When all tasks complete, you'll have:

- [ ] Spinnaker SDK 4.x.x installed on macOS ARM64
- [ ] Micro-Manager builds successfully
- [ ] Spinnaker4 adapter integrated
- [ ] Blackfly S camera detected
- [ ] Can capture images
- [ ] Live streaming works
- [ ] ROI operations work
- [ ] Hardware triggering works
- [ ] Performance meets specifications

## 🔍 Search Files

Looking for something specific?

| Keywords | Search In |
|----------|-----------|
| Build issues | All files (grep -r "build issue" .) |
| API changes | SPINNAKER_QUICK_REFERENCE.md |
| Boost compatibility | QUICK_DISABLE_BLUEBOX.md, SERIALMANAGER_BOOST_ASIO_FIX.md |
| Troubleshooting | QUICK_DISABLE_BLUEBOX.md, BUILD_STATUS_AND_NEXT_STEPS.md |
| Code examples | SPINNAKER_ADAPTER_TEMPLATE/README.md, Spinnaker4Camera.cpp |
| Patching | patches/README.md |
| Testing | test_spinnaker_env.sh |
| Implementation plan | SPINNAKER_MACOS_ARM64_PLAN.md |

## 💡 Pro Tips

1. **Start with README_FOR_YOU.md** - It points to everything you need
2. **Run test_spinnaker_env.sh first** - Catches issues early
3. **Use REBUILD_WITH_SPINNAKER.sh** - Automates the build
4. **Apply patches** - Easier than manual editing
5. **Read BUILD_STATUS_AND_NEXT_STEPS.md** - Detailed next actions
6. **Check SPINNAKER_QUICK_REFERENCE.md** - For specific issues

## 📞 Need Help?

| Problem | Solution File |
|----------|---------------|
| Can't build | BUILD_STATUS_AND_NEXT_STEPS.md |
| Need to disable adapter | QUICK_DISABLE_BLUEBOX.md or patches/ |
| SerialManager errors | SERIALMANAGER_BOOST_ASIO_FIX.md |
| Environment issues | test_spinnaker_env.sh |
| API questions | SPINNAKER_QUICK_REFERENCE.md |
| General overview | SPINNAKER_IMPLEMENTATION_SUMMARY.md |
| Full documentation | SPINNAKER_MACOS_ARM64_PLAN.md |

---

**Total Solution: 18 files, 147.9 KB**
**Ready for production use on macOS Apple Silicon with FLIR Spinnaker SDK 4.x.x**
