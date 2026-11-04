package service

import (
	"bytes"
	"fmt"
	"image"
	"image/jpeg"
	"image/png"
	"os"
	"path/filepath"

	"tebox-api/internal/constants"
	"tebox-api/internal/service/types"
	"tebox-api/internal/utils"
)

func (m *Manager) Preview(id string, opts types.PreviewOptions) (types.Preview, error) {
	item, err := m.Get(id)
	if err != nil {
		return types.Preview{}, err
	}
	if !item.Running {
		return types.Preview{}, constants.ErrInputNotRunning
	}
	format, mime, err := utils.NormalizePreviewFormat(opts.Format)
	if err != nil {
		return types.Preview{}, fmt.Errorf("%w: %v", constants.ErrInvalidOptions, err)
	}
	quality := opts.Quality
	if quality <= 0 {
		quality = constants.PreviewDefaultQuality
	}
	if quality > 100 {
		quality = 100
	}

	img, err := m.captureScreen(item)
	if err != nil {
		return types.Preview{}, err
	}
	// QEMU's screendump (egl_gl_readback) already rotates to upright using
	// the guest-reported rotation, so the dump is correctly oriented here.
	img = utils.ScalePreview(img, opts.Size)

	var buf bytes.Buffer
	switch format {
	case constants.PreviewFormatPNG:
		if err := png.Encode(&buf, img); err != nil {
			return types.Preview{}, err
		}
	default:
		if err := jpeg.Encode(&buf, img, &jpeg.Options{Quality: quality}); err != nil {
			return types.Preview{}, err
		}
	}
	b := img.Bounds()
	return types.Preview{
		Data:   buf.Bytes(),
		Format: format,
		Mime:   mime,
		Width:  b.Dx(),
		Height: b.Dy(),
	}, nil
}

func (m *Manager) captureScreen(item types.Instance) (image.Image, error) {
	_ = m
	dump := filepath.Join(item.Dir, "preview-dump.ppm")
	_ = os.Remove(dump)
	if err := (utils.QMPSocket{Path: utils.MonitorQMPPath(item.Dir)}).Screendump(dump, "ppm"); err != nil {
		return nil, fmt.Errorf("preview: qmp screendump failed: %w", err)
	}
	defer os.Remove(dump)
	data, err := os.ReadFile(dump)
	if err != nil {
		return nil, err
	}
	return utils.DecodePPM(data)
}
