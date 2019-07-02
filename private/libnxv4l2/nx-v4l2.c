/*
 * Copyright (c) 2016 Nexell Co., Ltd.
 * Author: Sungwoo, Park <swpark@nexell.co.kr>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>
#include <linux/media.h>

#include "nx-v4l2.h"

#define DEVNAME_SIZE			64
#define DEVNODE_SIZE			64

#define SYSFS_PATH_SIZE			128

#define MAX_CAMERA_INSTANCE_NUM		12
#define MAX_SUPPORTED_RESOLUTION	10

#define MAX_PLANES			3

struct nx_v4l2_entry {
	bool exist;
	bool is_mipi;
	bool interlaced;
	char devname[DEVNAME_SIZE];
	char sensorname[DEVNAME_SIZE];
	char devnode[DEVNODE_SIZE];
	int list_count;
	struct nx_v4l2_frame_info lists[MAX_SUPPORTED_RESOLUTION];
};

enum {
	type_category_subdev = 0,
	type_category_video = 1,
};

enum v4l2_interval {
	V4L2_INTERVAL_MIN = 0,
	V4L2_INTERVAL_MAX,
};

static struct nx_v4l2_entry_cache {
	bool cached;
	struct nx_v4l2_entry entries[nx_v4l2_max][MAX_CAMERA_INSTANCE_NUM];
} _nx_v4l2_entry_cache = {
	.cached	= false,
};

static void enum_all_supported_resolutions(struct nx_v4l2_entry *e);
static void print_all_supported_resolutions(struct nx_v4l2_entry *e);
static int enum_all_v4l2_devices(void);

static int get_type_category(uint32_t type)
{
	switch (type) {
#ifndef NOT_USED
	case nx_sensor_subdev:
	case nx_clipper_subdev:
	case nx_decimator_subdev:
	case nx_csi_subdev:
		return type_category_subdev;
#endif
	default:
		return type_category_video;
	}
}

static uint32_t get_buf_type(uint32_t type)
{
	switch (type) {
	case nx_clipper_video:
	case nx_decimator_video:
		return V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	default:
		return V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	}
}

static void print_nx_v4l2_entry(struct nx_v4l2_entry *e, int i)
{
	if (e->exist) {
		printf("\n");
		printf("[%d] devname\t:\t%s\n", i, e->devname);
		printf("devnode\t\t:\t%s\n", e->devnode);
		printf("is_mipi\t\t:\t%d\n", e->is_mipi);
		printf("interlaced\t:\t%d\n", e->interlaced);
		printf("list_count\t:\t%d\n", e->list_count);

		for(int i = 0; i< e->list_count ; i++)
		{
			printf("%d\t : width - %d\t height - %d\t\n", i, e->lists[i].width, e->lists[i].height);
		}

	}
}

void print_all_nx_v4l2_entry(void)
{
	int i, j;
	struct nx_v4l2_entry_cache *cache = &_nx_v4l2_entry_cache;
	struct nx_v4l2_entry *entry;

	if (!cache->cached)
		enum_all_v4l2_devices();

	for (j = 0; j < nx_v4l2_max; j++) {
		for (i = 0; i < MAX_CAMERA_INSTANCE_NUM; i++) {
			entry = &cache->entries[j][i];
			if (entry->exist)
				print_nx_v4l2_entry(entry, i);
		}
	}
}

static struct nx_v4l2_entry *find_v4l2_entry(int type, int module)
{
	struct nx_v4l2_entry_cache *cache = &_nx_v4l2_entry_cache;

	return &cache->entries[type][module];
}

#ifndef NOT_USED
#define NX_CLIPPER_SUBDEV_NAME		"nx-clipper"
#define NX_DECIMATOR_SUBDEV_NAME	"nx-decimator"
#define NX_CSI_SUBDEV_NAME		"nx-csi"
#define NX_MPEGTS_VIDEO_NAME		"VIDEO MPEGTS"
#endif
#define NX_CLIPPER_VIDEO_NAME		"VIDEO CLIPPER"
#define NX_DECIMATOR_VIDEO_NAME		"VIDEO DECIMATOR"
static int get_type_by_name(char *type_name)
{
#ifndef NOT_USED
	if (!strncmp(type_name, NX_CLIPPER_SUBDEV_NAME,
		     strlen(NX_CLIPPER_SUBDEV_NAME))) {
		return nx_clipper_subdev;
	} else if (!strncmp(type_name, NX_DECIMATOR_SUBDEV_NAME,
			    strlen(NX_DECIMATOR_SUBDEV_NAME))) {
		return nx_decimator_subdev;
	} else if (!strncmp(type_name, NX_CSI_SUBDEV_NAME,
			    strlen(NX_CSI_SUBDEV_NAME))) {
		return nx_csi_subdev;
	} else if (!strncmp(type_name, NX_MPEGTS_VIDEO_NAME,
			    strlen(NX_MPEGTS_VIDEO_NAME))) {
		return nx_mpegts_video;
	} else if (!strncmp(type_name, NX_CLIPPER_VIDEO_NAME,
			    strlen(NX_CLIPPER_VIDEO_NAME))) {
		return nx_clipper_video;
#else
	if (!strncmp(type_name, NX_CLIPPER_VIDEO_NAME,
				strlen(NX_CLIPPER_VIDEO_NAME))) {
		return nx_clipper_video;
#endif
	} else if (!strncmp(type_name, NX_DECIMATOR_VIDEO_NAME,
			    strlen(NX_DECIMATOR_VIDEO_NAME))) {
		return nx_decimator_video;
	} else {
		/* fprintf(stderr, "can't find type for name %s\n", type_name); */
		return -EINVAL;
	}
}
#ifndef NOT_USED
static int get_sensor_info(char *name, int *module)
{
	int i;
	struct nx_v4l2_entry *e = &_nx_v4l2_entry_cache.entries[nx_sensor_subdev][0];

	for (i = 0; i < MAX_CAMERA_INSTANCE_NUM; i++, e++) {
		if (e->exist &&
			!strncmp(name, e->devname, strlen(e->devname))) {
			*module = i;
			return nx_sensor_subdev;
		}
	}

	return -ENODEV;
}
#endif

static struct nx_v4l2_entry *find_v4l2_entry_by_name(char *name)
{
	int type;
	int module;
	char type_name[DEVNAME_SIZE] = {0, };
	char sensor_name[DEVNAME_SIZE] = {0, };

	memset(type_name, 0, DEVNAME_SIZE);
	memset(sensor_name, 0, DEVNAME_SIZE);

	sscanf(name, "%[^0-9]%d", type_name, &module);

	type = get_type_by_name(type_name);
	if (type < 0) {
#ifndef NOT_USED
		type = get_sensor_info(name, &module);
		if (type < 0)
			return NULL;
#else
		return NULL;
#endif
	}
	return find_v4l2_entry(type, module);
}

/* camera sensor sysfs entry
 * /sys/devices/platform/camerasensor[MODULE]/info
 * ex> /sys/devices/platform/camerasensor0/info
 */
static void enum_camera_sensor(void)
{
	int sys_fd;
	int i;
	char sysfs_path[64] = {0, };
#ifndef NOT_USED
	struct nx_v4l2_entry *e = &_nx_v4l2_entry_cache.entries[nx_sensor_subdev][0];
#else
	struct nx_v4l2_entry *e = &_nx_v4l2_entry_cache.entries[nx_clipper_video][0];
#endif
	for (i = 0; i < MAX_CAMERA_INSTANCE_NUM; i++, e++) {
		sprintf(sysfs_path,
			"/sys/devices/platform/camerasensor%d/info",i);
		sys_fd = open(sysfs_path, O_RDONLY);
		if (sys_fd >= 0) {
			char buf[512] = {0, };
			int size = read(sys_fd, buf, sizeof(buf));

			close(sys_fd);
			if (size < 0) {
				fprintf(stderr, "failed to read %s\n",
					sysfs_path);
			} else {
				if (strcmp("no exist", buf)) {
					char *c;

					c = &buf[strlen("is_mipi:")];
					e->is_mipi = *c - '0';
					c += 13; /* ,interlaced: */
					e->interlaced = *c - '0';
					c += 7; /* ,name: */
#ifndef NOT_USED
					strncpy(e->devname, c, DEVNAME_SIZE);
#else
					strncpy(e->sensorname, c, DEVNAME_SIZE);
#endif
					e->exist = true;
				}
			}
		}
	}
}

static int enum_all_v4l2_devices(void)
{
	char *cur_dir;
	struct dirent **items;
	int nitems;
	int i;

	cur_dir = getcwd(NULL, 0);
	chdir("/sys/class/video4linux");

	enum_camera_sensor();

	nitems = scandir(".", &items, NULL, alphasort);
	for (i = 0; i < nitems; i++) {
		struct stat fstat;
		char entry_sys_path[SYSFS_PATH_SIZE];
		char entry_name[DEVNAME_SIZE];
		int fd;
		int read_count;
		struct nx_v4l2_entry *e;

		if ((!strcmp(items[i]->d_name, ".")) ||
		    (!strcmp(items[i]->d_name, "..")))
			continue;

		if (lstat(items[i]->d_name, &fstat) < 0) {
			fprintf(stderr, "lstat %s ", items[i]->d_name);
			continue;
		}

		memset(entry_sys_path, 0, SYSFS_PATH_SIZE);
		memset(entry_name, 0, DEVNAME_SIZE);

		sprintf(entry_sys_path, "%s/name", items[i]->d_name);

		fd = open(entry_sys_path, O_RDONLY);
		if (fd < 0) {
			fprintf(stderr, "can't open %s", entry_sys_path);
			continue;
		}

		read_count = read(fd, entry_name, DEVNAME_SIZE - 1);
		close(fd);

		if (read_count <= 0) {
			fprintf(stderr, "can't read %s\n", entry_sys_path);
			free(items[i]);
			continue;
		}
		e = find_v4l2_entry_by_name(entry_name);
		if (!e) {
			free(items[i]);
			continue;
		}
		e->exist = true;
		strcpy(e->devname, entry_name);
		sprintf(e->devnode, "/dev/%s", items[i]->d_name);
		if ((get_type_by_name(entry_name) == nx_clipper_video) ||
				(get_type_by_name(entry_name) == nx_decimator_video))
			enum_all_supported_resolutions(e);
		free(items[i]);
	}

	if (cur_dir) {
		chdir(cur_dir);
		free(cur_dir);
	}
	if (items)
		free(items);

	_nx_v4l2_entry_cache.cached = true;

	return 0;
}

/****************************************************************
 * public api
 */
int nx_v4l2_open_device(int type, int module)
{
	struct nx_v4l2_entry *entry = NULL;

	if (!_nx_v4l2_entry_cache.cached)
		nx_v4l2_enumerate();
#ifndef NOT_USED
	if (type == nx_csi_subdev)
		module = 0;
#endif
	entry = find_v4l2_entry(type, module);
	if (entry) {
		int fd = open(entry->devnode, O_RDWR);

		if (fd < 0)
			fprintf(stderr, "open failed for %s\n", entry->devname);

		return fd;
	} else {
		fprintf(stderr, "can't find device for type %d, module %d\n",
			type, module);
		return -ENODEV;
	}

}

void nx_v4l2_cleanup(void)
{
	struct nx_v4l2_entry_cache *cache = &_nx_v4l2_entry_cache;

	cache->cached = false;
}

bool nx_v4l2_is_mipi_camera(int module)
{
	struct nx_v4l2_entry *e;
#ifndef NOT_USED
	e = find_v4l2_entry(nx_sensor_subdev, module);
#else
	e = find_v4l2_entry(nx_clipper_video, module);
#endif
	if (e && e->is_mipi)
		return true;

	return false;
}

bool nx_v4l2_is_interlaced_camera(int module)
{
	struct nx_v4l2_entry *e;

#ifndef NOT_USED
	e = find_v4l2_entry(nx_sensor_subdev, module);
#else
	e = find_v4l2_entry(nx_clipper_video, module);
#endif
	if (e && e->interlaced)
		return true;

	return false;
}

int nx_v4l2_link(bool link, int module, int src_type, int src_pad,
	int sink_type, int sink_pad)
{
	/* currently not used, just left for build */
	return 0;
}

static int subdev_set_format(int fd, uint32_t w, uint32_t h, uint32_t format)
{
	struct v4l2_subdev_format fmt;
	bzero(&fmt, sizeof(fmt));
	fmt.pad = 0;
	fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
	fmt.format.code = format;
	fmt.format.width = w;
	fmt.format.height = h;
	fmt.format.field = V4L2_FIELD_NONE;
	return ioctl(fd, VIDIOC_SUBDEV_S_FMT, &fmt);
}

static int video_set_format(int fd, uint32_t w, uint32_t h, uint32_t format,
			    uint32_t buf_type)
{
	struct v4l2_format v4l2_fmt;

	bzero(&v4l2_fmt, sizeof(v4l2_fmt));
	v4l2_fmt.type = buf_type;
	v4l2_fmt.fmt.pix_mp.width = w;
	v4l2_fmt.fmt.pix_mp.height = h;
	v4l2_fmt.fmt.pix_mp.pixelformat = format;
	v4l2_fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
	return ioctl(fd, VIDIOC_S_FMT, &v4l2_fmt);
}

static int video_set_format_with_field(int fd, uint32_t w, uint32_t h, uint32_t format,
			    uint32_t buf_type, uint32_t field)
{
	struct v4l2_format v4l2_fmt;

	bzero(&v4l2_fmt, sizeof(v4l2_fmt));
	v4l2_fmt.type = buf_type;
	v4l2_fmt.fmt.pix_mp.width = w;
	v4l2_fmt.fmt.pix_mp.height = h;
	v4l2_fmt.fmt.pix_mp.pixelformat = format;
	v4l2_fmt.fmt.pix_mp.field = field;
	return ioctl(fd, VIDIOC_S_FMT, &v4l2_fmt);
}

static int video_set_format_mmap(int fd, uint32_t w, uint32_t h,
				 uint32_t format, uint32_t buf_type)
{
	struct v4l2_format v4l2_fmt;

	bzero(&v4l2_fmt, sizeof(v4l2_fmt));
	v4l2_fmt.type = buf_type;
	v4l2_fmt.fmt.pix.width = w;
	v4l2_fmt.fmt.pix.height = h;
	v4l2_fmt.fmt.pix.pixelformat = format;
	v4l2_fmt.fmt.pix.field = V4L2_FIELD_ANY;
	return ioctl(fd, VIDIOC_S_FMT, &v4l2_fmt);
}

int nx_v4l2_set_fmt(int fd, uint32_t f, uint32_t w, uint32_t h,
		uint32_t num_planes, uint32_t strides[], uint32_t sizes[])
{
	struct v4l2_format v4l2_fmt;


	bzero(&v4l2_fmt, sizeof(struct v4l2_format));

	v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

	v4l2_fmt.fmt.pix_mp.width = w;
	v4l2_fmt.fmt.pix_mp.height = h;
	v4l2_fmt.fmt.pix_mp.pixelformat = f;
	v4l2_fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;
	v4l2_fmt.fmt.pix_mp.num_planes = num_planes;
	for (uint32_t i = 0; i < num_planes; i++) {
		struct v4l2_plane_pix_format *plane_fmt;
		plane_fmt = &v4l2_fmt.fmt.pix_mp.plane_fmt[i];
		plane_fmt->sizeimage = sizes[i];
		plane_fmt->bytesperline = strides[i];
	}

	return ioctl(fd, VIDIOC_S_FMT, &v4l2_fmt);
}

#ifndef NOT_USED
int nx_v4l2_set_format(int fd, int type, uint32_t w, uint32_t h,
	uint32_t format)
{
	if (get_type_category(type) == type_category_subdev)
		return subdev_set_format(fd, w, h, format);
	else
		return video_set_format(fd, w, h, format, get_buf_type(type));
}

int nx_v4l2_set_format_with_field(int fd, int type, uint32_t w, uint32_t h,
	uint32_t format, uint32_t field)
{
	if (get_type_category(type) == type_category_subdev)
		return subdev_set_format(fd, w, h, format);
	else
		return video_set_format_with_field(fd, w, h, format, get_buf_type(type), field);
}
#endif

int nx_v4l2_set_format_mmap(int fd, int type, uint32_t w, uint32_t h,
			    uint32_t format)
{
	if (get_type_category(type) == type_category_subdev)
		return subdev_set_format(fd, w, h, format);
	else
		return video_set_format_mmap(fd, w, h, format,
					     V4L2_BUF_TYPE_VIDEO_CAPTURE);
}

static int subdev_get_format(int fd, uint32_t *w, uint32_t *h, uint32_t *format)
{
	int ret;
	struct v4l2_subdev_format fmt;

	bzero(&fmt, sizeof(fmt));

	ret = ioctl(fd, VIDIOC_SUBDEV_G_FMT, &fmt);
	if (ret)
		return ret;

	*w = fmt.format.width;
	*h = fmt.format.height;
	*format = fmt.format.code;

	return 0;
}

static int video_get_format(int fd, uint32_t *w, uint32_t *h, uint32_t *format,
			    uint32_t buf_type)
{
	int ret;
	struct v4l2_format v4l2_fmt;

	bzero(&v4l2_fmt, sizeof(v4l2_fmt));
	v4l2_fmt.type = buf_type;
	v4l2_fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
	ret = ioctl(fd, VIDIOC_G_FMT, &v4l2_fmt);
	if (ret)
		return ret;

	*w = v4l2_fmt.fmt.pix_mp.width;
	*h = v4l2_fmt.fmt.pix_mp.height;
	*format = v4l2_fmt.fmt.pix_mp.pixelformat;
	return 0;
}

int nx_v4l2_get_format(int fd, int type, uint32_t *w, uint32_t *h,
		       uint32_t *format)
{
	if (get_type_category(type) == type_category_subdev)
		return subdev_get_format(fd, w, h, format);
	else
		return video_get_format(fd, w, h, format, get_buf_type(type));
}

static int subdev_set_crop(int fd, uint32_t x, uint32_t y, uint32_t w,
			   uint32_t h)
{
	struct v4l2_subdev_crop crop;

	bzero(&crop, sizeof(crop));
	crop.rect.left = x;
	crop.rect.top = y;
	crop.rect.width = w;
	crop.rect.height = h;
	return ioctl(fd, VIDIOC_SUBDEV_S_CROP, &crop);
}

static int video_set_crop(int fd, uint32_t x, uint32_t y, uint32_t w,
			  uint32_t h, uint32_t buf_type)
{
	struct v4l2_crop crop;

	bzero(&crop, sizeof(crop));
	crop.type = buf_type;
	crop.c.left = x;
	crop.c.top = y;
	crop.c.width = w;
	crop.c.height = h;
	return ioctl(fd, VIDIOC_S_CROP, &crop);
}

int nx_v4l2_set_crop(int fd, int type, uint32_t x, uint32_t y,
		     uint32_t w, uint32_t h)
{
	if (get_type_category(type) == type_category_subdev)
		return subdev_set_crop(fd, x, y, w, h);
	else
		return video_set_crop(fd, x, y, w, h, get_buf_type(type));
}

int nx_v4l2_set_crop_mmap(int fd, int type, uint32_t x, uint32_t y,
			  uint32_t w, uint32_t h)
{
	if (get_type_category(type) == type_category_subdev)
		return subdev_set_crop(fd, x, y, w, h);
	else
		return video_set_crop(fd, x, y, w, h,
				      V4L2_BUF_TYPE_VIDEO_CAPTURE);
}

static int subdev_get_crop(int fd, uint32_t *x, uint32_t *y, uint32_t *w,
			   uint32_t *h)
{
	int ret;
	struct v4l2_subdev_crop crop;

	bzero(&crop, sizeof(crop));
	ret = ioctl(fd, VIDIOC_SUBDEV_G_CROP, &crop);
	if (ret)
		return ret;

	*x = crop.rect.left;
	*y = crop.rect.top;
	*w = crop.rect.width;
	*h = crop.rect.height;
	return 0;
}

static int video_get_crop(int fd, uint32_t *x, uint32_t *y, uint32_t *w,
			  uint32_t *h, uint32_t buf_type)
{
	int ret;
	struct v4l2_crop crop;

	bzero(&crop, sizeof(crop));
	crop.type = buf_type;
	ret = ioctl(fd, VIDIOC_G_CROP, &crop);
	if (ret)
		return ret;

	*x = crop.c.left;
	*y = crop.c.top;
	*w = crop.c.width;
	*h = crop.c.height;
	return 0;
}

int nx_v4l2_get_crop(int fd, int type, uint32_t *x, uint32_t *y, uint32_t *w,
		     uint32_t *h)
{
	if (get_type_category(type) == type_category_subdev)
		return subdev_get_crop(fd, x, y, w, h);
	else
		return video_get_crop(fd, x, y, w, h, get_buf_type(type));
}

int nx_v4l2_set_ctrl(int fd, int type, uint32_t ctrl_id, int value)
{
	struct v4l2_control ctrl;

	bzero(&ctrl, sizeof(ctrl));
	ctrl.id = ctrl_id;
	ctrl.value = value;

	return ioctl(fd, VIDIOC_S_CTRL, &ctrl);
}

int nx_v4l2_get_ctrl(int fd, int type, uint32_t ctrl_id, int *value)
{
	int ret;
	struct v4l2_control ctrl;

	bzero(&ctrl, sizeof(ctrl));
	ctrl.id = ctrl_id;
	ret = ioctl(fd, VIDIOC_G_CTRL, &ctrl);
	if (ret)
		return ret;
	*value = ctrl.value;
	return 0;
}

int nx_v4l2_set_ext_ctrl(int fd, uint32_t ctrl_id, void *arg)
{
	struct v4l2_ext_control ext_ctrl;
	struct v4l2_ext_controls ext_ctrls;

	bzero(&ext_ctrl, sizeof(ext_ctrl));
	bzero(&ext_ctrls, sizeof(ext_ctrls));

	ext_ctrl.id = ctrl_id;
	ext_ctrl.ptr = arg;
	ext_ctrls.controls = &ext_ctrl;

	return ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_ctrls);
}

int nx_v4l2_get_ext_ctrl(int fd, uint32_t ctrl_id, void *arg)
{
	int ret;
	struct v4l2_ext_controls ext_ctrl;

	bzero(&ext_ctrl, sizeof(ext_ctrl));
	ext_ctrl.controls->id = ctrl_id;
	ret = ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_ctrl);
	if (ret)
		return ret;

	arg = ext_ctrl.controls->ptr;
	return 0;
}

int nx_v4l2_reqbuf(int fd, int type, int count)
{
	struct v4l2_requestbuffers req;

	if (get_type_category(type) == type_category_subdev)
		return -EINVAL;

	bzero(&req, sizeof(req));
	req.count = count;
	req.memory = V4L2_MEMORY_DMABUF;
	req.type = get_buf_type(type);
	return ioctl(fd, VIDIOC_REQBUFS, &req);
}

int nx_v4l2_reqbuf_mmap(int fd, int type, int count)
{
	struct v4l2_requestbuffers req;

	if (get_type_category(type) == type_category_subdev)
		return -EINVAL;

	bzero(&req, sizeof(req));
	req.count = count;
	req.memory = V4L2_MEMORY_MMAP;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	return ioctl(fd, VIDIOC_REQBUFS, &req);
}

int nx_v4l2_qbuf(int fd, int type, int plane_num, int index, int *fds,
		 int *sizes)
{
	struct v4l2_buffer v4l2_buf;
	struct v4l2_plane planes[MAX_PLANES];
	int i;

	if (get_type_category(type) == type_category_subdev)
		return -EINVAL;

	if (plane_num > MAX_PLANES) {
		fprintf(stderr, "plane_num(%d) is over MAX_PLANES\n",
			plane_num);
		return -EINVAL;
	}

	bzero(&v4l2_buf, sizeof(v4l2_buf));
	v4l2_buf.m.planes = planes;
	v4l2_buf.type = get_buf_type(type);
	v4l2_buf.memory = V4L2_MEMORY_DMABUF;
	v4l2_buf.index = index;
	v4l2_buf.length = plane_num;
	for (i = 0; i < plane_num; i++) {
		v4l2_buf.m.planes[i].m.fd = fds[i];
		v4l2_buf.m.planes[i].length = sizes[i];
	}

	return ioctl(fd, VIDIOC_QBUF, &v4l2_buf);
}

int nx_v4l2_qbuf_mmap(int fd, int type, int index)
{
	struct v4l2_buffer v4l2_buf;

	if (get_type_category(type) == type_category_subdev)
		return -EINVAL;

	bzero(&v4l2_buf, sizeof(v4l2_buf));
	v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4l2_buf.memory = V4L2_MEMORY_MMAP;
	v4l2_buf.index = index;

	return ioctl(fd, VIDIOC_QBUF, &v4l2_buf);
}

int nx_v4l2_dqbuf(int fd, int type, int plane_num, int *index)
{
	int ret;
	struct v4l2_buffer v4l2_buf;
	struct v4l2_plane planes[MAX_PLANES];

	if (get_type_category(type) == type_category_subdev)
		return -EINVAL;

	if (plane_num > MAX_PLANES) {
		fprintf(stderr, "plane_num(%d) is over MAX_PLANES\n",
			plane_num);
		return -EINVAL;
	}

	bzero(&v4l2_buf, sizeof(v4l2_buf));
	v4l2_buf.m.planes = planes;
	v4l2_buf.type = get_buf_type(type);
	v4l2_buf.memory = V4L2_MEMORY_DMABUF;
	v4l2_buf.length = plane_num;

	ret = ioctl(fd, VIDIOC_DQBUF, &v4l2_buf);
	if (ret)
		return ret;

	*index = v4l2_buf.index;

	return 0;
}

int nx_v4l2_dqbuf_with_timestamp(int fd, int type, int plane_num, int *index,
				 struct timeval *timeval)
{
	int ret;
	struct v4l2_buffer v4l2_buf;
	struct v4l2_plane planes[MAX_PLANES];

	if (get_type_category(type) == type_category_subdev)
		return -EINVAL;

	if (plane_num > MAX_PLANES) {
		fprintf(stderr, "plane_num(%d) is over MAX_PLANES\n",
			plane_num);
		return -EINVAL;
	}

	bzero(&v4l2_buf, sizeof(v4l2_buf));
	v4l2_buf.m.planes = planes;
	v4l2_buf.type = get_buf_type(type);
	v4l2_buf.memory = V4L2_MEMORY_DMABUF;
	v4l2_buf.length = plane_num;

	ret = ioctl(fd, VIDIOC_DQBUF, &v4l2_buf);
	if (ret)
		return ret;

	*index = v4l2_buf.index;

	memcpy(timeval, &v4l2_buf.timestamp, sizeof(*timeval));

	return 0;
}

int nx_v4l2_dqbuf_mmap(int fd, int type, int *index)
{
	int ret;
	struct v4l2_buffer v4l2_buf;

	if (get_type_category(type) == type_category_subdev)
		return -EINVAL;

	bzero(&v4l2_buf, sizeof(v4l2_buf));
	v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4l2_buf.memory = V4L2_MEMORY_MMAP;

	ret = ioctl(fd, VIDIOC_DQBUF, &v4l2_buf);
	if (ret)
		return ret;

	*index = v4l2_buf.index;

	return 0;
}

int nx_v4l2_dqbuf_mmap_with_timestamp(int fd, int type, int *index,
				      struct timeval *timeval)
{
	int ret;
	struct v4l2_buffer v4l2_buf;

	if (get_type_category(type) == type_category_subdev)
		return -EINVAL;

	bzero(&v4l2_buf, sizeof(v4l2_buf));
	v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4l2_buf.memory = V4L2_MEMORY_MMAP;

	ret = ioctl(fd, VIDIOC_DQBUF, &v4l2_buf);
	if (ret)
		return ret;

	*index = v4l2_buf.index;

	memcpy(timeval, &v4l2_buf.timestamp, sizeof(*timeval));

	return 0;
}

int nx_v4l2_streamon(int fd, int type)
{
	uint32_t buf_type;

	if (get_type_category(type) == type_category_subdev)
		return -EINVAL;

	buf_type = get_buf_type(type);
	return ioctl(fd, VIDIOC_STREAMON, &buf_type);
}

int nx_v4l2_streamon_mmap(int fd, int type)
{
	uint32_t buf_type;

	if (get_type_category(type) == type_category_subdev)
		return -EINVAL;

	buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	return ioctl(fd, VIDIOC_STREAMON, &buf_type);
}

int nx_v4l2_streamoff(int fd, int type)
{
	uint32_t buf_type;
	if (get_type_category(type) == type_category_subdev)
		return -EINVAL;

	buf_type = get_buf_type(type);
	return ioctl(fd, VIDIOC_STREAMOFF, &buf_type);
}

int nx_v4l2_streamoff_mmap(int fd, int type)
{
	uint32_t buf_type;
	if (get_type_category(type) == type_category_subdev)
		return -EINVAL;

	buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	return ioctl(fd, VIDIOC_STREAMOFF, &buf_type);
}

int nx_v4l2_query_buf_mmap(int fd, int type, int index,
			   struct v4l2_buffer *v4l2_buf)
{

	if (get_type_category(type) == type_category_subdev)
		return -EINVAL;

	bzero(v4l2_buf, sizeof(*v4l2_buf));
	v4l2_buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4l2_buf->memory = V4L2_MEMORY_MMAP;
	v4l2_buf->index = index;

	return ioctl(fd, VIDIOC_QUERYBUF, v4l2_buf);
}

int nx_v4l2_set_parm(int fd, int type, struct v4l2_streamparm *parm)
{
	parm->type = get_buf_type(type);
	return ioctl(fd, VIDIOC_S_PARM, parm);
}

int nx_v4l2_get_framesize(int fd, struct nx_v4l2_frame_info *f)
{
	struct v4l2_frmsizeenum frame;
	int ret = 0;

	frame.index = f->index;
	ret = ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frame);
	if (!ret) {
		f->width = frame.stepwise.max_width;
		f->height = frame.stepwise.max_height;
	}
	return ret;
}

int nx_v4l2_get_frameinterval(int fd, struct nx_v4l2_frame_info *f, int minOrMax)
{
	struct v4l2_frmivalenum frame;
	int ret;

	frame.index = minOrMax;
	frame.width = f->width;
	frame.height = f->height;
	ret = ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frame);
	if (!ret) {
		f->interval[frame.index] = frame.discrete.denominator;
	}
	return ret;
}

static void enum_all_supported_resolutions(struct nx_v4l2_entry *e)
{
	int i, j, ret;
	struct nx_v4l2_frame_info *f;
	int fd = open(e->devnode, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "can't open %s", e->devnode);
		return;
	}
	for (i = 0; i < MAX_SUPPORTED_RESOLUTION; i++) {
		f = &e->lists[i];
		f->index = i;
		ret = nx_v4l2_get_framesize(fd, f);
		if (!ret) {
			for (j = 0; j <= V4L2_INTERVAL_MAX; j++) {
				ret = nx_v4l2_get_frameinterval(fd, f, j);
				if (ret) {
					printf("Failed to get interval for width:%d, height:%d\n",
						f->width, f->height);
					e->list_count = i;
					close(fd);
					return;
				}
			}
		} else {
			e->list_count = i;
			break;
		}
	}
	close(fd);
}

static void print_all_supported_resolutions(struct nx_v4l2_entry *e)
{
	printf("sensorname\t:\t%s\n", e->sensorname);
	for (int i = 0; i < e->list_count; i++) {
		struct nx_v4l2_frame_info *f = &e->lists[i];

		printf("[%d] width:%d, height:%d, interval min:%d max:%d\n",
				i, f->width, f->height,
				f->interval[0], f->interval[1]);
	}
}

void nx_v4l2_print_all_video_entry(void)
{
	int i, j;
	struct nx_v4l2_entry_cache *cache = &_nx_v4l2_entry_cache;
	struct nx_v4l2_entry *entry;

	if (!cache->cached)
		enum_all_v4l2_devices();

	for (j = nx_clipper_video; j <= nx_decimator_video; j++) {
		for (i = 0; i < MAX_CAMERA_INSTANCE_NUM; i++) {
			entry = &cache->entries[j][i];
			if (entry->exist) {
#ifndef NOT_USED
				entry->is_mipi = nx_v4l2_is_mipi_camera(i);
				entry->interlaced = nx_v4l2_is_interlaced_camera(i);
				strcpy(entry->sensorname, cache->entries[nx_sensor_subdev][i].devname);
#endif
				print_nx_v4l2_entry(entry, i);
				print_all_supported_resolutions(entry);
			}
		}
	}
}

void nx_v4l2_enumerate(void)
{
	if (_nx_v4l2_entry_cache.cached == false) {
		enum_all_v4l2_devices();
	}
	nx_v4l2_print_all_video_entry();
}

char *nx_v4l2_get_video_path(int type, uint32_t module)
{
	struct nx_v4l2_entry_cache *cache = &_nx_v4l2_entry_cache;
	struct nx_v4l2_entry *entry;

	if (!cache->cached) {
		enum_all_v4l2_devices();
	}
	entry = &cache->entries[type][module];
	if (entry->exist)
		return entry->devnode;
	return NULL;
}


void nx_v4l2_get_resolution(int type, int module, int* width, int* height)
{
	struct nx_v4l2_entry_cache *cache = &_nx_v4l2_entry_cache;
	struct nx_v4l2_entry *entry;

	if (!cache->cached)
		enum_all_v4l2_devices();
	
	entry = &cache->entries[type][module];

	*width = entry->lists[0].width;
	*height = entry->lists[0].height;
}
