
# 给树莓派写操作系统
2016年的时候，我把[EPOS](https://github.com/hongmingjian/epos)移植到了树莓派上，完成了中断、基本的内存管理和多线程。为了防止自己将来忘掉这个过程，就就写了一个初稿，但只写到了内存管理。本来想把读写SD卡的功能加上，然后继续写后面的内容。但是，那时候没有找到SD控制器相关的参考资料，而且来了一波工作上的事情，就把这个事给放下了。

2020年初，武汉肺炎扩散到全国的时候，宅在家里查了点资料把SD卡读写的功能加上，顺带也让EPOS可以在QEMU里面运行了。4年之后的今天发现，目前可查到的树莓派CPU(博通BCM28XX)的资料比2016年多了很多，关于GPU、USB、网卡、SD控制器等等。总的感觉是，官方公布的少，网友自己扒出来的多。感谢各路网友的深扒，想在树莓派上面搞点事情的难度也降低了。

2016年的时候，一开始就在Word里面写。写到后面，需要大量引用源代码，效率非常低下，但也不想中途换工具，硬着头皮往下写。为了提高效率，这次把原来Word的内容移植到LaTeX里面来，大大提高了写作效率。

# Writing an operating system for Raspberry Pi
It was 2016 when I tried to write this book. After finishing 4 chapters, I stopped writing because of other business.
