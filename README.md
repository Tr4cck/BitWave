# Bit Wave

![](src/resources/assets/logo-full.svg)

建立在 MPV, Qt 与 Microsoft Fluent Design System 之上的音视频播放器。

## Developing

developing env requirements:
```
$ sudo pacman -Syu mpv qt5 ffmpeg cmake
```

```
$ git clone https://github.com/Reverier-Xu/BitWave.git && cd BitWave

BitWave $ mkdir build && cd build

BitWave/build $ cmake .. && make

BitWave/build $ ./bin/BitWave
```

## Status

- 支持播放绝大部分格式的各种音频文件。
- 完善的视频播放支持。
- 支持播放网易云音乐专有格式(ncm)。
- 支持获取媒体的metadata与封面图片，能够自动探测封面图片的主色调。
- 好看的界面。

## ScreenShots

![image.png](https://i.loli.net/2021/08/30/9B5DjOXR8uHKAme.png)

![image.png](https://i.loli.net/2021/08/30/2t74vKisSfbJl9o.png)

![image.png](https://i.loli.net/2021/08/30/ELjMKmfZVQN5sUJ.png)

![image.png](https://i.loli.net/2021/08/30/Hun5ckty1lDRGZB.png)

![image.png](https://i.loli.net/2021/08/30/AfJnXCY9ztcUqrg.png)
