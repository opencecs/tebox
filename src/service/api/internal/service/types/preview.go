package types

// PreviewOptions selects screendump encode parameters.
type PreviewOptions struct {
	Size    int    // longest side in pixels; 0 keeps native
	Format  string // jpeg (default) or png
	Quality int    // 1..100, jpeg only; default 80
}

// Preview is an encoded guest framebuffer still.
type Preview struct {
	Data   []byte
	Format string
	Mime   string
	Width  int
	Height int
}
