DEPENDPATH *= $$system_path($$clean_path($${PWD}/../../libbitdht/src))
INCLUDEPATH  *= $$system_path($$clean_path($${PWD}/../../libbitdht/src))
LIBS *= -L$$system_path($$clean_path($${OUT_PWD}/../../libbitdht/src/lib/)) -lbitdht

!equals(TARGET, bitdht):PRE_TARGETDEPS *= $$system_path($$clean_path($${OUT_PWD}/../../libbitdht/src/lib/libbitdht.a))
