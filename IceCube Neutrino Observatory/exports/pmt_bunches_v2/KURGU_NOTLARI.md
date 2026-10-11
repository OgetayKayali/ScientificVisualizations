# PMT — görünür elektron demetleri, v2

24 saniye · 30 fps · 3840 × 2160 ana çıktı ve 1920 × 1080 önizleme. Sessiz MP4; kesin ses senkronu henüz yapılmadı.

## Bu sürümde değişenler

- Elektronlar aynı noktada üst üste doğup yolun sonunda açılmaz. Dinotun farklı noktalarından çok kısa zaman aralıklarıyla çıkan, birlikte ilerleyen sıkı demetler olarak çizilir.
- Elektronların çevresinde koyu ayırıcı sınır bulunur. Her elektrona geniş bir parlama eklemek yerine keskin noktalar kullanılır; demetin bir ışık lekesine dönüşmesi önlenir.
- Dinotlardan çıkan temsili parçacık sayıları sırasıyla 4, 9, 18, 32, 52, 80, 116, 162, 222 ve 300'dür. Eski sürümde üst sınır 25'ti. Son demette 12 kat daha fazla parçacık çizilir.
- Son demet anotun farklı yüksekliklerindeki toplama noktalarına ulaşır. Tüm elektronlar tek bir noktaya yığılmaz.
- Dile bağlı başlık ve açıklamalar kaldırıldı. PMT, γ, e⁻, ΔV ve ×10⁶–10⁸ kullanıldı. Lacivert ve buz mavisi tasarım sürdürüldü; gelen foton Güneş sahnesiyle uyumlu sıcak renktedir.
- Çıkış grafiği tüpün altında ayrı boşlukta yer alır.

## Zamanlama

| Zaman | Görsel |
|---|---|
| 0–4,8 sn | Foton fotokatoda gelir; bir fotoelektron çıkar. |
| 4,8–8,3 sn | Gerilim vurgusu ve dinotların belirginleşmesi. |
| 8,3–12,1 sn | Elektron ilk dinoda hızlanır; ilk ikincil elektron demeti çıkar. |
| 12,1–17,8 sn | Demetler dinottan dinoda ilerlerken sayıca ve hacimce büyür. |
| 17,8–24 sn | Anotta toplanma, kazanç ve çıkış darbesi. |

Çizilen parçacık sayıları, elektrot boyutları, demet dağılımı ve uçuş süreleri öğretici bir görselleştirmedir; sayısal PMT simülasyonu değildir. Gerçek milyonlarca elektronun tamamı gösterilmez. ×10⁶–10⁸, kullanıcının genel PMT kazancı anlatımına karşılık gelir; modelin temsili 300 noktasından hesaplanmaz. Fotoelektronun kısa süre beklemesi anlatımla eşleme içindir. Darbenin biçimi ve polaritesi temsili olup ölçüm verisi değildir.

Anlatım metni SRT ve JSON'da korunmuştur. Kaynak kod `animation/icecube_samples/render_pmt_v2.py`; ortak çizim altyapısı `motion_common.py`. Yeniden üretim: `render_pmt_v2.py --scale 2`.

Mekanizma kaynağı: [IceCube: fotokatot, dinotlar ve anot](https://icecube.wisc.edu/news/research/2020/02/optimizing-eyeballs-of-icecube-neutrino-observatory/).
