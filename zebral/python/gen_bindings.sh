binder --root-module zebral --prefix python/ --bind zebral python/all_includes.hpp --config python/zebral.cfg -- -std=c++20 -I. -Icamera/inc -Icommon/inc -Iserial/inc -DNDEBUG

