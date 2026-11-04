package service

import (
	"image"
	"image/color"
	"testing"

	"tebox-api/internal/utils"
)

func TestFitPreviewSize(t *testing.T) {
	cases := []struct {
		srcW, srcH, maxSize, wantW, wantH int
	}{
		{1080, 2400, 0, 1080, 2400},
		{1080, 2400, 2400, 1080, 2400},
		{1080, 2400, 1200, 540, 1200},
		{1080, 2400, 540, 243, 540},
		{1080, 1920, 960, 540, 960},
		{100, 50, 200, 200, 100},
		{100, 100, 8000, 4096, 4096},
	}
	for _, c := range cases {
		gotW, gotH := utils.FitPreviewSize(c.srcW, c.srcH, c.maxSize)
		if gotW != c.wantW || gotH != c.wantH {
			t.Fatalf("FitPreviewSize(%d,%d,%d)=%d,%d want %d,%d",
				c.srcW, c.srcH, c.maxSize, gotW, gotH, c.wantW, c.wantH)
		}
	}
}

func TestDecodePPMAndScale(t *testing.T) {
	ppm := []byte("P6\n2 1\n255\n" + string([]byte{255, 0, 0, 0, 0, 255}))
	img, err := utils.DecodePPM(ppm)
	if err != nil {
		t.Fatal(err)
	}
	if img.Bounds().Dx() != 2 || img.Bounds().Dy() != 1 {
		t.Fatalf("size %v", img.Bounds())
	}
	r, g, b, a := img.At(0, 0).RGBA()
	if r>>8 != 255 || g>>8 != 0 || b>>8 != 0 || a>>8 != 255 {
		t.Fatalf("pixel0 %d %d %d %d", r, g, b, a)
	}

	scaled := utils.ScalePreview(img, 4)
	if scaled.Bounds() != image.Rect(0, 0, 4, 2) {
		t.Fatalf("scaled %v", scaled.Bounds())
	}
	if _, _, _, a := scaled.At(0, 0).RGBA(); a>>8 != 255 {
		t.Fatal("scaled alpha")
	}
}

func TestNormalizePreviewFormat(t *testing.T) {
	format, mime, err := utils.NormalizePreviewFormat("")
	if err != nil || format != "jpeg" || mime != "image/jpeg" {
		t.Fatalf("default %s %s %v", format, mime, err)
	}
	format, mime, err = utils.NormalizePreviewFormat("PNG")
	if err != nil || format != "png" || mime != "image/png" {
		t.Fatalf("png %s %s %v", format, mime, err)
	}
	if _, _, err := utils.NormalizePreviewFormat("webp"); err == nil {
		t.Fatal("expected invalid format")
	}
}

func TestScalePreviewIdentity(t *testing.T) {
	src := image.NewRGBA(image.Rect(0, 0, 3, 4))
	src.Set(1, 1, color.RGBA{10, 20, 30, 255})
	got := utils.ScalePreview(src, 0)
	if got != src {
		t.Fatal("native size should keep the same image")
	}
}
