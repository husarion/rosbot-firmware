# Changelog

All notable changes to this project are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)
and this project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html)
for the `X.Y.Z` portion of each tag.

Release tags carry a track suffix (`-jazzy` for the micro-ROS track,
`-jazzy-mavlink` for the MAVLink track, etc.). Versions are independent
per track — `v0.1.0-jazzy-mavlink` and `v1.1.0-jazzy` are not comparable
across the suffix boundary, they are parallel release lineages until
MAVLink merges back into `jazzy`.

## [0.1.1-jazzy-mavlink] - 2026-05-18

### Added
- Release pipeline now publishes `rosbot_mavlink_bridge` container images to GHCR alongside the firmware artifacts.

### Fixed
- Release workflow no longer aborts under `set -u` while assembling the `rosbot_mavlink_bridge` tarball, so the bridge artifact is produced reliably.

## [0.1.0-jazzy-mavlink] - 2026-05-18

### Added
- MAVLink firmware as a second transport alongside the micro-ROS firmware; both rosbot and rosbot_xl can be flashed with either flavour. Pairs with the new `rosbot_mavlink_bridge` on the SBC side.
- `just` recipes plus `scripts/flash.sh` wrapper for the SBC-side build + flash workflow; flash picks the right model and port based on the PlatformIO env and points at the freshly-built `firmware.bin`.
- `just release` recipe with tag-driven release workflow — bumps the version, updates the changelog, commits, tags and pushes; auto-bootstraps the dev venv (PlatformIO included) on a clean host.

### Changed
- `ROS_API.md` rewritten to be transport-neutral, so the same topic/service contract covers both the micro-ROS firmware and the MAVLink firmware + `rosbot_mavlink_bridge` pair.
