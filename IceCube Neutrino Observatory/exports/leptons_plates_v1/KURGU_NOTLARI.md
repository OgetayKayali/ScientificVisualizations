# Nötrino → elektron / müon → metal plaka benzetmesi

İki paragraf, kesmesiz tek bir 82 saniyelik animasyonda birleştirildi. 3840 × 2160, 30 fps, sessiz H.264 MP4; ayrıca 1920 × 1080 izleme kopyası.

## Akış ve zamanlama

| Zaman | Görsel anlatım |
|---|---|
| 0–7,2 sn | Çok sayıda nötrino maddeyi geçer; bir etkileşme ve ikincil parçacıklar vurgulanır. |
| 7,2–11 sn | Ayrı bir ikinci etkileşmede müon oluşur; üst sırada elektron, alt sırada müon. |
| 11–14,5 sn | e⁻ sembolü ve negatif yük vurgusu. |
| 14,5–20,2 sn | m(μ⁻) ≈ 207 m(e⁻). Çizilen parçacıkların çapı bu oranla büyütülmez. |
| 20,2–36 sn | Elektronun kısa ve dallanan izi ile müonun daha uzun izi karşılaştırılır. |
| 35,4–40,6 sn | Atomlar konumlarını koruyarak metal plakalara dönüşür. İkinci paragraf 36. saniyede başlar. |
| 41,5–46 sn | Plakaların üzerinde kısa süre atom simgeleri görünür; benzetmenin eşleştirmesi kurulur. |
| 46–50,5 sn | e⁻ ve 9 mm; küçük temsili mermi. |
| 50,5–60,2 sn | İlk plakalardan geçiş, yön değişimi ve parçaların dağılması. |
| 60,2–66,2 sn | Mermi ve plaka parçaları aynı biçim ve renkle karışır. |
| 66,2–72,5 sn | μ⁻ ve .50; daha büyük temsili mermi. |
| 72,5–78,7 sn | Plaka dizisi boyunca daha düz geçiş; küçük delikler ve az sayıda parça. |
| 78,7–82 sn | Büyük mermi μ⁻ işaretine dönüşür; iki iz birlikte tutulur. |

SRT ve JSON'da kullanıcının metni cümle bazında korunmuştur. Toplam 141 yazılı kelime, yaklaşık 103 kelime/dakika üzerinden zamanlandı; sayıların okunması ve kısa duraklar için pay bırakıldı. Kayıtlı seslendirme ile kesin senkron henüz yapılmadı.

## Görsel dil

Lacivert zemin, elektron için buz mavisi, müon için maviye yakın açık lavanta. e⁻ ve μ⁻ simgeleri renk ayrımına ek olarak her sıranın kimliğini korur. Dile bağlı açıklama yazıları kullanılmaz. Sayılar, fizik sembolleri ve kalibre gösterimleri evrensel işaretler olarak yer alır. İki paragraf arasındaki bağlantı, aynı atom/plaka konumları ve aynı iki yatay sıra üzerinden kurulur.

## Bilimsel ve temsili sınırlar

- Elektron ve müon iki ayrı nötrino etkileşmesiyle gösterilir. Bir nötrinonun tek olayda zorunlu olarak hem elektron hem müon ürettiği ima edilmez. Gelen nötrinoların e ve μ alt işaretleri iki örnek kanalı ayırır.
- Nötrinoların çoğu etkileşmeden geçer. Görülen iki olay öğretici olarak seçilmiştir; ekrandaki nötrino sayısı gerçek etkileşme olasılığı değildir.
- Müon/elektron kütle oranı yaklaşık 207'dir. Müon, 207 elektronun birleşimi olarak veya 207 kat büyük geometrik çapla gösterilmez.
- Elektronun dallanan izi elektromanyetik sağanağın, müonun uzun izi daha nüfuz edici hareketinin nitel bir temsilidir. Müon da enerji kaybeder ve etkileşir. Gerçek menzil enerjiye ve ortama bağlıdır; kütle tek başına menzili belirlemez.
- Atom çizimleri simgeseldir; ölçekli atom modelleri değildir.
- Plaka ve mermi bölümü kullanıcının benzetmesidir. Gerçek balistik simülasyonu, zırh/mermi performans testi veya belirli sayıda plakanın delineceğine ilişkin veri değildir. Plaka sayısı, kalınlığı, malzemesi, delikler, sapma açıları ve parçalanma dekoratif/temsili olarak seçilmiştir. Silah kullanımı veya işleyişi gösterilmez.
- Süreler, parçacık boyutları ve hareket hızları bilimsel ölçekli değildir. Nötrino salınımları ve bütün etkileşme ürünleri bu açıklamanın kapsamı dışında bırakılmıştır.

## Kaynaklar ve yeniden üretim

- [IceCube Masterclass: elektron nötrinosu sağanağı ve müon nötrinosu izi](https://masterclass.icecube.wisc.edu/en/icecube-and-neutrinos/detecting-neutrinos/).
- [IceCube: track ve cascade olayları](https://icecube.wisc.edu/news/research/2019/11/what-can-cascade-events-tell-us-about-neutrino-sources/).
- [NIST CODATA: müon/elektron kütle oranı](https://physics.nist.gov/cgi-bin/cuu/Results?search_for=muon).
- [Particle Data Group: madde içinden parçacık geçişi](https://pdg.lbl.gov/2024/reviews/experimental_methods_and_colliders.html).

Kaynak kod: `animation/icecube_samples/render_leptons_plates.py`; ortak altyapı `motion_common.py`. `render_leptons_plates.py --scale 2` 4K ve 1080p kopyaları üretir; `--preview` yalnızca kontrol karelerini oluşturur.

Tüm görseller bu proje için kodla çizilmiştir. Harici stok görsel, ses veya sentetik seslendirme yoktur.
