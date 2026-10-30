Build
```sh
flatpak-builder --force-clean --user --install-deps-from=flathub --repo=repo --install builddir io.github.triadwoozie.fiveinvader.json
```

Bundle
```sh
flatpak build-bundle repo io.github.triadwoozie.fiveinvader.flatpak io.github.triadwoozie.fiveinvader --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo
```
