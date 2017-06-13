CONFIG(debug, debug|release) {
    EXTRA_IMPORT_PATHS += $${PWD}/bin/debug/
} else {
    EXTRA_IMPORT_PATHS += $${PWD}/bin/release/
}

