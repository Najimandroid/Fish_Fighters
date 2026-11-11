param([string]$out, [string]$ringOut, [switch]$title)
# The Fish Tank hub, redrawn from the first sketch (backgrounds/menus/fish_tank(old).png, 1280x720) at 1920x1080 in
# the units' doodle style: dark wobbly outlines, flat fills. A submarine room: ceiling and floor strips, a hull wall
# in riveted plates, a pipe with a gauge and a valve wheel, and the big porthole on the right (water in three bands,
# light rays, the ring's shadow on the glass). The ring is also written alone (ringOut) so the game can draw fish
# behind it (UIFishTank: its glass and ring numbers match CX, CY, GLASS_RX, GLASS_RY and the ring box printed here).
#
# Usage (Windows PowerShell, from the project folder):
#   powershell -File tools/art/hub_background.ps1 -out assets/images/textures/backgrounds/menus/fish_tank.png -ringOut assets/images/textures/backgrounds/menus/porthole_ring.png
#   powershell -File tools/art/hub_background.ps1 -out assets/images/textures/backgrounds/menus/title.png -title   (the calm room of the title screen)
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @"
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;

public static class HubArt
{
    const int W = 1920, H = 1080;
    static readonly Color OUTLINE = Color.FromArgb(16, 24, 34);
    static readonly Color WALL = Color.FromArgb(146, 156, 172);
    static readonly Color WALL_ALT = Color.FromArgb(139, 149, 166);
    static readonly Color STRIP = Color.FromArgb(167, 176, 192);
    static readonly Color SEAM = Color.FromArgb(112, 123, 141);
    static readonly Color RING = Color.FromArgb(172, 182, 198);
    static readonly Color RING_DARK = Color.FromArgb(128, 139, 157);
    static readonly Color WATER_TOP = Color.FromArgb(155, 226, 255);
    static readonly Color WATER_MID = Color.FromArgb(92, 181, 217);
    static readonly Color WATER_LOW = Color.FromArgb(40, 147, 190);
    static readonly Color WATER_DEEP = Color.FromArgb(30, 120, 165);

    // Porthole (the game reads the same numbers: UIManager's hub tank)
    public const float CX = 1392f, CY = 556f, RING_RX = 302f, RING_RY = 258f, GLASS_RX = 236f, GLASS_RY = 204f;

    // Smooth deterministic noise in [-1, 1]
    static double Noise(double t, double seed)
    {
        return 0.5 * Math.Sin(t * 1.3 + seed) + 0.3 * Math.Sin(t * 2.9 + seed * 1.7) + 0.2 * Math.Sin(t * 6.1 + seed * 2.3);
    }

    // A hand-drawn line through the points: subdivided, nudged sideways by the noise
    static PointF[] Wobble(PointF[] points, float amplitude, double seed, float step = 10f)
    {
        var result = new List<PointF>();
        double travelled = 0;
        for (int i = 0; i + 1 < points.Length; i++)
        {
            PointF a = points[i], b = points[i + 1];
            float dx = b.X - a.X, dy = b.Y - a.Y, len = (float)Math.Sqrt(dx * dx + dy * dy);
            if (len < 1e-3f) continue;
            float nx = -dy / len, ny = dx / len;
            int count = Math.Max(1, (int)(len / step));
            for (int k = 0; k < count; k++)
            {
                float f = (float)k / count;
                double n = Noise((travelled + f * len) / 60.0, seed) * amplitude;
                result.Add(new PointF(a.X + dx * f + nx * (float)n, a.Y + dy * f + ny * (float)n));
            }
            travelled += len;
        }
        PointF last = points[points.Length - 1];
        result.Add(new PointF(last.X + (float)(Noise(travelled / 60.0, seed) * amplitude) * 0f, last.Y));
        return result.ToArray();
    }

    static PointF[] Ellipse(float cx, float cy, float rx, float ry, float amplitude, double seed, int count = 200)
    {
        var p = new PointF[count];
        for (int i = 0; i < count; i++)
        {
            double a = 2 * Math.PI * i / count;
            double r = 1 + amplitude * Noise(a * 3, seed) / Math.Max(rx, ry);
            p[i] = new PointF(cx + (float)(Math.Cos(a) * rx * r), cy + (float)(Math.Sin(a) * ry * r));
        }
        return p;
    }

    static Pen Line(float width, Color color)
    {
        var pen = new Pen(color, width);
        pen.LineJoin = LineJoin.Round; pen.StartCap = LineCap.Round; pen.EndCap = LineCap.Round;
        return pen;
    }

    static void Shape(Graphics g, PointF[] p, Color fill, float width = 5f)
    {
        using (var b = new SolidBrush(fill)) g.FillPolygon(b, p);
        using (var pen = Line(width, OUTLINE)) g.DrawPolygon(pen, p);
    }

    // A doodled rivet: a small circle, a highlight dot
    static void Rivet(Graphics g, float x, float y, double seed, float r = 6f, Color? line = null)
    {
        var p = Ellipse(x, y, r, r, 0.8f, seed, 24);
        using (var b = new SolidBrush(Color.FromArgb(60, 255, 255, 255))) g.FillPolygon(b, p);
        using (var pen = Line(3f, line ?? SEAM)) g.DrawPolygon(pen, p);
    }

    // The wavy edge between the wall and a strip (y at x)
    static PointF[] Edge(float[] ys, double seed)
    {
        var pts = new PointF[ys.Length];
        for (int i = 0; i < ys.Length; i++) pts[i] = new PointF(-20 + i * (W + 40f) / (ys.Length - 1), ys[i]);
        return Wobble(pts, 3f, seed, 12f);
    }

    static void DrawRoom(Graphics g)
    {
        g.Clear(WALL);

        // Hull plates: alternating tones, seams with rivets (the sketch's wall, now riveted)
        float[] columns = { 0, 330, 700, 1080, 1480, 1920 };
        float[] rows = { 120, 540, 980 };
        for (int c = 0; c + 1 < columns.Length; c++)
            for (int r = 0; r + 1 < rows.Length; r++)
                if ((c + r) % 2 == 1)
                    using (var b = new SolidBrush(WALL_ALT)) g.FillRectangle(b, columns[c], rows[r], columns[c + 1] - columns[c], rows[r + 1] - rows[r]);
        using (var seam = Line(4f, SEAM))
        {
            for (int c = 1; c + 1 < columns.Length; c++)
            {
                g.DrawLines(seam, Wobble(new[] { new PointF(columns[c], 110), new PointF(columns[c], 990) }, 2f, c * 3.1));
                for (float y = 150; y < 960; y += 46) Rivet(g, columns[c] + 16, y, c * 7 + y);
            }
            g.DrawLines(seam, Wobble(new[] { new PointF(-10, rows[1]), new PointF(W + 10, rows[1]) }, 2f, 9.7));
            for (float x = 30; x < W; x += 46) Rivet(g, x, rows[1] + 16, x * 0.37);
        }

        // Ceiling and floor strips (the sketch's light bands), the floor's deck grating
        var ceiling = new List<PointF> { new PointF(-20, -20) };
        ceiling.AddRange(Edge(new float[] { 116, 120, 126, 131, 133, 130, 126, 122, 118, 117, 119, 114, 110 }, 1.3));
        ceiling.Add(new PointF(W + 20, -20));
        Shape(g, ceiling.ToArray(), STRIP);

        var floorEdge = Edge(new float[] { 996, 998, 994, 990, 985, 983, 980, 976, 972, 970, 971, 975, 982 }, 4.1);
        var floor = new List<PointF>(floorEdge);
        floor.Add(new PointF(W + 20, H + 20)); floor.Add(new PointF(-20, H + 20));
        Shape(g, floor.ToArray(), STRIP);
        using (var grate = Line(3f, SEAM))
            for (float x = 24; x < W; x += 42)
                g.DrawLines(grate, Wobble(new[] { new PointF(x, 1010), new PointF(x - 14, H + 10) }, 1.2f, x));
        using (var lip = Line(3f, SEAM))
            g.DrawLines(lip, Wobble(new[] { new PointF(-10, 1006), new PointF(W + 10, 994) }, 2f, 6.6));
    }

    // A pipe from the floor up to the ceiling, an elbow running right under the ceiling, a gauge and a valve
    static void DrawPipes(Graphics g)
    {
        Color pipe = Color.FromArgb(118, 132, 152), pipeLight = Color.FromArgb(150, 163, 182);
        float px = 840, pw = 44;
        // Ceiling run
        Shape(g, Wobble(new[] { new PointF(px, 150), new PointF(W + 20, 150), new PointF(W + 20, 194), new PointF(px + pw, 194), new PointF(px + pw, 150) }, 1.5f, 2.2), pipe);
        // Vertical run (drawn over the ceiling run's end: the elbow)
        Shape(g, Wobble(new[] { new PointF(px, 150), new PointF(px + pw, 150), new PointF(px + pw, 1000), new PointF(px, 1000), new PointF(px, 150) }, 1.5f, 3.3), pipe);
        var elbow = Ellipse(px + pw / 2, 172, 36, 36, 1f, 5.5, 60);
        Shape(g, elbow, pipeLight);
        using (var hi = Line(5f, Color.FromArgb(120, 255, 255, 255)))
        {
            g.DrawLine(hi, px + 11, 230, px + 11, 970);
            g.DrawLine(hi, px + 70, 161, W, 161);
        }
        // Flanges with bolts
        foreach (float fy in new float[] { 320, 760 })
        {
            Shape(g, Wobble(new[] { new PointF(px - 10, fy), new PointF(px + pw + 10, fy), new PointF(px + pw + 10, fy + 22), new PointF(px - 10, fy + 22), new PointF(px - 10, fy) }, 1f, fy), pipeLight);
            Rivet(g, px - 2, fy + 11, fy, 4f, OUTLINE); Rivet(g, px + pw + 2, fy + 11, fy + 1, 4f, OUTLINE);
        }
        foreach (float fx in new float[] { 1200, 1700 })
        {
            Shape(g, Wobble(new[] { new PointF(fx, 140), new PointF(fx + 22, 140), new PointF(fx + 22, 204), new PointF(fx, 204), new PointF(fx, 140) }, 1f, fx), pipeLight);
        }

        // Pressure gauge on a short stem: white face, ticks, the needle in the red
        float gx = 948, gy = 452, gr = 58;
        Shape(g, Wobble(new[] { new PointF(px + pw, gy - 9), new PointF(gx - gr + 4, gy - 9), new PointF(gx - gr + 4, gy + 9), new PointF(px + pw, gy + 9), new PointF(px + pw, gy - 9) }, 0.8f, 7.7), pipe, 4f);
        Shape(g, Ellipse(gx, gy, gr, gr, 1.2f, 8.8, 80), Color.FromArgb(128, 140, 160));
        Shape(g, Ellipse(gx, gy, gr - 12, gr - 12, 1f, 9.9, 80), Color.FromArgb(245, 247, 240), 4f);
        using (var red = new Pen(Color.FromArgb(230, 80, 70), 9f))
            g.DrawArc(red, gx - gr + 20, gy - gr + 20, (gr - 20) * 2, (gr - 20) * 2, -40, 55);
        using (var tick = Line(3f, OUTLINE))
            for (int i = 0; i <= 8; i++)
            {
                double a = Math.PI * (0.75 + 1.5 * i / 8.0);
                g.DrawLine(tick, gx + (float)Math.Cos(a) * (gr - 22), gy + (float)Math.Sin(a) * (gr - 22), gx + (float)Math.Cos(a) * (gr - 16), gy + (float)Math.Sin(a) * (gr - 16));
            }
        using (var needle = Line(5f, OUTLINE))
            g.DrawLine(needle, gx, gy, gx + (float)Math.Cos(-0.35) * (gr - 20), gy + (float)Math.Sin(-0.35) * (gr - 20));
        Shape(g, Ellipse(gx, gy, 7, 7, 0f, 1, 20), Color.FromArgb(230, 80, 70), 3f);

        // Valve wheel near the floor
        float vx = px + pw / 2, vy = 880, vr = 46;
        using (var spoke = Line(7f, OUTLINE))
            for (int i = 0; i < 3; i++)
            {
                double a = Math.PI * i / 3 + 0.3;
                g.DrawLine(spoke, vx - (float)Math.Cos(a) * vr, vy - (float)Math.Sin(a) * vr, vx + (float)Math.Cos(a) * vr, vy + (float)Math.Sin(a) * vr);
            }
        using (var wheelOutline = Line(17f, OUTLINE)) g.DrawPolygon(wheelOutline, Ellipse(vx, vy, vr, vr, 1f, 3.7, 60));
        using (var wheel = Line(9f, Color.FromArgb(230, 80, 70))) g.DrawPolygon(wheel, Ellipse(vx, vy, vr, vr, 1f, 3.7, 60));
        Shape(g, Ellipse(vx, vy, 10, 10, 0f, 1, 20), Color.FromArgb(230, 80, 70), 4f);
    }

    // The ring alone (the game draws it again over the fish swimming in the glass)
    public static void DrawRing(Graphics g)
    {
        var outer = Ellipse(CX, CY, RING_RX, RING_RY, 5f, 1.9);
        var inner = Ellipse(CX - 4, CY - 2, GLASS_RX, GLASS_RY, 3f, 2.7);
        using (var path = new GraphicsPath())
        {
            path.AddPolygon(outer); path.AddPolygon(inner);   // Alternate fill: the hole stays empty
            using (var b = new SolidBrush(RING)) g.FillPath(b, path);
        }
        // The lip around the glass, darker, then the outlines
        using (var lip = Line(14f, RING_DARK)) g.DrawPolygon(lip, Ellipse(CX - 4, CY - 2, GLASS_RX + 10, GLASS_RY + 10, 3f, 2.7));
        using (var pen = Line(6f, OUTLINE)) { g.DrawPolygon(pen, outer); g.DrawPolygon(pen, inner); }
        using (var hi = Line(6f, Color.FromArgb(110, 255, 255, 255)))
            g.DrawArc(hi, CX - RING_RX + 22, CY - RING_RY + 18, (RING_RX - 22) * 2, (RING_RY - 18) * 2, 200, 70);
        // Bolts on the ring
        for (int i = 0; i < 10; i++)
        {
            double a = 2 * Math.PI * i / 10 + 0.2;
            float bx = CX + (float)Math.Cos(a) * (RING_RX + GLASS_RX + 16) / 2, by = CY + (float)Math.Sin(a) * (RING_RY + GLASS_RY + 16) / 2;
            Shape(g, Ellipse(bx, by, 10, 10, 1f, i * 1.3, 24), Color.FromArgb(150, 161, 178), 3.5f);
            using (var slot = Line(3f, OUTLINE)) g.DrawLine(slot, bx - 5, by + 1, bx + 5, by - 1);
        }
    }

    static void DrawWater(Graphics g)
    {
        var glass = Ellipse(CX - 4, CY - 2, GLASS_RX + 6, GLASS_RY + 6, 3f, 2.7);
        using (var clip = new GraphicsPath())
        {
            clip.AddPolygon(glass);
            g.SetClip(clip);
            g.Clear(WATER_TOP);
            float left = CX - GLASS_RX - 20, right = CX + GLASS_RX + 20;
            // Two wavy bands (the sketch's), a deep floor with sand and weeds
            Func<float, float, double, PointF[]> wave = (y, amp, seed) =>
            {
                var p = new List<PointF>();
                for (float x = left; x <= right; x += 8) p.Add(new PointF(x, y + amp * (float)Noise(x / 45.0, seed)));
                p.Add(new PointF(right, CY + 400)); p.Add(new PointF(left, CY + 400));
                return p.ToArray();
            };
            using (var b = new SolidBrush(WATER_MID)) g.FillPolygon(b, wave(CY - 110, 22, 1.1));
            using (var b = new SolidBrush(WATER_LOW)) g.FillPolygon(b, wave(CY + 90, 14, 2.4));
            using (var b = new SolidBrush(WATER_DEEP)) g.FillPolygon(b, wave(CY + 150, 10, 3.6));
            // Light rays from the top left
            using (var ray = new SolidBrush(Color.FromArgb(34, 255, 255, 255)))
                foreach (float rx in new float[] { -150, -40, 90 })
                    g.FillPolygon(ray, new[] { new PointF(CX + rx, CY - 260), new PointF(CX + rx + 70, CY - 260), new PointF(CX + rx + 200, CY + 260), new PointF(CX + rx + 90, CY + 260) });
            // Weeds on the bottom
            using (var weed = Line(9f, Color.FromArgb(40, 120, 90)))
                foreach (float wx in new float[] { CX - 170, CX - 150, CX + 120, CX + 150, CX + 175 })
                    g.DrawCurve(weed, new[] { new PointF(wx, CY + 230), new PointF(wx + 10, CY + 190), new PointF(wx - 6, CY + 150), new PointF(wx + 8, CY + 120 + (wx % 3) * 6) });
            // Doodled bubbles
            using (var bub = Line(3f, Color.FromArgb(200, 240, 255, 255)))
                foreach (var b in new[] { new PointF(CX + 150, CY - 60), new PointF(CX + 165, CY - 100), new PointF(CX + 158, CY - 135), new PointF(CX - 120, CY + 20), new PointF(CX - 110, CY - 14) })
                    g.DrawEllipse(bub, b.X - 7, b.Y - 7, 14, 14);
            // The ring's shadow on the glass (top), the glass's gleam (top left)
            using (var shade = Line(26f, Color.FromArgb(60, 16, 24, 34)))
                g.DrawArc(shade, CX - GLASS_RX - 4, CY - GLASS_RY + 6, GLASS_RX * 2, GLASS_RY * 2, 190, 160);
            using (var gleam = Line(8f, Color.FromArgb(200, 255, 255, 255)))
            {
                g.DrawArc(gleam, CX - GLASS_RX + 40, CY - GLASS_RY + 34, (GLASS_RX - 40) * 2, (GLASS_RY - 34) * 2, 205, 32);
                g.DrawArc(gleam, CX - GLASS_RX + 62, CY - GLASS_RY + 56, (GLASS_RX - 62) * 2, (GLASS_RY - 56) * 2, 212, 16);
            }
            g.ResetClip();
        }
    }

    // The title screen: the same room, calm (no pipes, no porthole: the logo and START stand on it)
    public static void RenderTitle(string outPath)
    {
        using (var bmp = new Bitmap(W, H, PixelFormat.Format32bppArgb))
        using (var g = Graphics.FromImage(bmp))
        {
            g.SmoothingMode = SmoothingMode.AntiAlias;
            g.PixelOffsetMode = PixelOffsetMode.HighQuality;
            DrawRoom(g);
            bmp.Save(outPath, ImageFormat.Png);
        }
    }

    public static void Render(string outPath, string ringPath)
    {
        using (var bmp = new Bitmap(W, H, PixelFormat.Format32bppArgb))
        using (var g = Graphics.FromImage(bmp))
        {
            g.SmoothingMode = SmoothingMode.AntiAlias;
            g.PixelOffsetMode = PixelOffsetMode.HighQuality;
            DrawRoom(g);
            DrawPipes(g);
            DrawWater(g);
            DrawRing(g);
            bmp.Save(outPath, ImageFormat.Png);
        }
        using (var bmp = new Bitmap(W, H, PixelFormat.Format32bppArgb))
        using (var g = Graphics.FromImage(bmp))
        {
            g.SmoothingMode = SmoothingMode.AntiAlias;
            g.PixelOffsetMode = PixelOffsetMode.HighQuality;
            g.Clear(Color.Transparent);
            DrawRing(g);
            // Only the ring's box is kept (the game places it at its offset)
            int x0 = (int)(CX - RING_RX - 16), y0 = (int)(CY - RING_RY - 16), w = (int)(RING_RX * 2 + 32), h = (int)(RING_RY * 2 + 32);
            using (var crop = bmp.Clone(new Rectangle(x0, y0, w, h), PixelFormat.Format32bppArgb)) crop.Save(ringPath, ImageFormat.Png);
            Console.WriteLine("ring box " + x0 + " " + y0 + " " + w + " " + h);
        }
    }
}
"@
if ($title) { [HubArt]::RenderTitle($out) } else { [HubArt]::Render($out, $ringOut) }
