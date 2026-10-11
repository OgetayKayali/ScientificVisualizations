# Işığın vakumdaki ve sudaki hızı

40 saniye, 3840 × 2160, 30 fps, sessiz H.264 MP4. Ayrıca 1920 × 1080 izleme kopyası. Önceki sahnelerle aynı lacivert, buz mavisi ve açık lavanta paleti.

| Zaman | Görsel |
|---|---|
| 0–5 sn | Ortak başlangıçtan ilerleyen ışık ve müon; müon c referansının gerisinde. |
| 5–9,5 sn | c ≈ 300 000 km/s. |
| 9,5–14,2 sn | Alt sırada su hacmi belirir; iki ışık işareti eşzamanlı ilerler. |
| 14,2–21 sn | Alt işaret suya girince hızı 0,75c olur. Su sınırına normal girişte yol doğrultusu değişmez. |
| 21–26 sn | Vakumdaki c yeniden vurgulanır. |
| 26–29,3 sn | Su hacmi genişler; karşılaştırma üç sıraya dönüşür. |
| 29,3–37,3 sn | Aynı başlangıçtan, aynı anda: vakum ışığı c, müon temsilen 0,90c, su ışığı yaklaşık 0,75c. |
| 37,3–40 sn | Son konumlar ve 0.75c < v(μ⁻) < c karşılaştırması tutulur. |

Tüm hız karşılaştırmalarında hareket doğrusaldır; sabit hız sırasında easing kullanılmaz. Easing yalnızca sahne geçişlerinde kullanılır. Son yarışta aynı 8 saniyede alınan grafik mesafeler sırasıyla 1160, 1044 ve 870 mantıksal pikseldir. İkinci sahnede iki işaret su sınırına kadar aynı hızdadır; alt işaretin yalnızca su içindeki hareketi %25 yavaşlatılır. Hareket sonunda öğretici bir donuk kare tutulur.

Dile bağlı açıklama yazıları yoktur. γ genel foton simgesi, μ⁻ müon simgesidir; γ kullanımı ışığın gama enerjisinde olduğu anlamına gelmez. Su H₂O, damla ve silik molekül simgeleriyle ayrılır. Dalga biçimi ışık darbesinin temsili işaretidir; optik frekans gerçek zaman ölçeğinde gösterilmez.

Su için n ≈ 1,33 ve v ≈ c/n ≈ 0,75c, görünür ışık için yaklaşık bir anlatımdır. Kırılma indisi dalga boyuna ve koşullara bağlıdır. Müon için seçilen 0,90c yalnızca karşılaştırmayı okunur kılan temsili bir hızdır; bütün müonların sabit hızı değildir. Madde içindeki enerji kaybı ve sonraki Cherenkov yayımı bu paragrafın kapsamı dışında bırakılmıştır. Animasyon dur-kalk soğurma/yeniden yayılma modeli kullanmaz.

Zamanlama kullanıcının metnine göre tahminidir. Kayıtlı seslendirme ile henüz kesin senkron yapılmamıştır. Metin ayrı SRT ve timing.json dosyalarında korunmuştur; videoya gömülmez.

Kaynaklar:

- [NIST: vakumdaki ışık hızının SI tanımı](https://www.nist.gov/pml/special-publication-330/sp-330-section-2)
- [OpenStax: suyun kırılma indisi ve v = c/n](https://openstax.org/books/college-physics-2e/pages/25-3-the-law-of-refraction)
- [IceCube: maddesel ortamdaki ışığı geçen yüklü parçacıklar](https://icecube.wisc.edu/news/glashow-event-faq/)

Yeniden üretim: animation/icecube_samples/render_light_speed.py --scale 2. Ortak çizim altyapısı motion_common.py. Görseller kodla üretilmiştir; harici stok görsel veya seslendirme kullanılmamıştır.
