"""PandA-bambu machine-readable CI result protocol."""

LEGACY_SCHEMA_VERSION = "1.0"
MULTI_TASK_SCHEMA_VERSION = "1.1"
SUPPORTED_SCHEMA_VERSIONS = (LEGACY_SCHEMA_VERSION, MULTI_TASK_SCHEMA_VERSION)

# The existing open-build-only producer remains a 1.0 producer.
SCHEMA_VERSION = LEGACY_SCHEMA_VERSION
