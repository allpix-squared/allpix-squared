# Create Development Visualization with Gource:

* Captions with UNIX timestamps are located in `captions.txt`
* Gource configuration is stored in `gource.conf`
* To generate the `.ppm` output file, run:

    ```shell
    gource --load-config etc/gource/gource.conf
    ```

* To comvert to thw WebM format, use `ffmpeg`:

    ```shell
    ffmpeg -y -r 60 -f image2pipe -vcodec ppm -i gource_visualization.ppm -vcodec libvpx -b 10000K gource.webm
    ```

