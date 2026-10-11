# Cherenkov animasyonu — v2

32 saniye, 3840 × 2160, 30 fps, sessiz H.264 MP4. Ayrıca 1920 × 1080 izleme kopyası. Önceki sürüm ayrı klasörde korunmuştur.

Kullanıcının düzeltmesine göre 15–19. saniyelerdeki seçilmiş yayım noktası ve ileri açılan ışık doğrultuları kaldırıldı. Üç boyutlu son sahne artık ilk iki boyutlu sahnedeki geriye açılan dalga zarfını devam ettiriyor. Müon sağdaki tepe noktasında, koninin geniş kesiti solda kalır; hareket oku sağa bakar.

Koni yüzeyindeki tüm hareketli ışık noktaları, kısa izler, iç radyal çizgiler ve ara kesit halkaları kaldırıldı. Sade yarı saydam yüzey, çevre çizgisi ve siluet kenarları bırakıldı. Kamera 21,2–26 saniyeleri arasında hafifçe döner; ardından sabit kalır. Yüzey üzerinde dönme veya spiral hareketi yoktur.

Geometri, u/v = 5/6 olan aynı dalga zarfını iki ve üç boyutta korur: geriye açılan zarfın yarı açısı arcsin(u/v) ≈ 56,44°'dir. Son sahne fotonların açısal yayım doğrultularını gösteren ayrı bir çizim değil, ilk sahnenin üç boyutlu dalga zarfıdır. Görselleştirme temsilidir.

| Zaman | Görsel |
|---|---|
| 0–9,5 sn | Uçak, büyüyen ses dalgaları ve ortak zarf. |
| 8,8–11 sn | Uçaktan müona ve su ortamına yumuşak geçiş. |
| 11–19,5 sn | Aynı geriye açılan dalga zarfı korunur; ters yöne açılan ara gösterim yoktur. |
| 18,8–26 sn | Aynı tepe noktasından üç boyutlu koniye geçiş ve hafif kamera dönüşü. |
| 26–32 sn | Sabit açıdan sade koni yüzeyi. |

Metin ve toplam süre korunmuştur. SRT ve timing.json cümle zamanlamasını içerir; video altyazısız ve sessizdir. Zamanlama, kayıtlı seslendirmeyle henüz kesin eşlenmemiştir.

Kaynak kod: animation/icecube_samples/render_cherenkov_v2.py. Yeniden üretim: render_cherenkov_v2.py --scale 2.
