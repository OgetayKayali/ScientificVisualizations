# Güneş’in içinden gelen nötrinolar

34 saniye, 30 fps. Ana çıktı 3840 × 2160 H.264 MP4; ayrıca 1920 × 1080 izleme kopyası. Ses ve gömülü altyazı yoktur.

## Görsel anlatım

Lacivert zemin ve buz mavisi ışık dili, onaylanan IceCube sahnesiyle ortaktır. Güneş’in tanınması ve iki parçacık yolunun ayrılması için sıcak sarı/kehribar kullanılır. Foton dalga paketi ve γ; nötrino doğrusal hareket ve ν ile gösterilir. Algılama kesitindeki geri tepen elektron e⁻ sembolüyle belirtilir. Hiçbir konuşma diline ait ekran yazısı kullanılmaz.

| Zaman | Anlatım | Görüntü |
|---|---|---|
| 0–6,8 sn | Merkezden doğrudan ışık alamıyoruz | Güneş, açılan kesit ve içeride yön değiştiren γ. |
| 6,8–12 sn | Yoğun ortam ve ışık-madde etkileşmesi | Kesite yakınlaşma, kısa ve kırık foton yolu, etkileşme noktaları. |
| 12–17,8 sn | Çekirdekteki nükleer reaksiyonlar ve nötrinolar | Çekirdekte doğum vurgusu; ν’nün dışarı doğru hareketi. |
| 17,8–20,6 sn | Rahatça geçebiliyor | ν katmanları geçer; Dünya belirir. |
| 20,6–23,4 sn | Onları yakalayabiliyoruz | Dedektörün büyütülmüş kesiti; ender ν–elektron etkileşmesi ve optik sensörlerde sinyal. |
| 23,4–28,2 sn | İç bölgedeki süreçler hakkında | Çekirdek, nötrino yolu ve algılama aynı kadrajda. |
| 28,2–34 sn | Yalnızca ışıkla edinemediğimiz bilgi | Yön değiştiren γ ve doğrudan çıkan ν birlikte görünür; sinyal tutuşu. |

Zamanlama sağlanan paragrafa göre tahminidir. SRT ve JSON cümle sınırları kurgu referansıdır. Kayıtlı seslendirme ile kesin eşleme henüz yapılmadı.

## Bilimsel temsil

- Çekirdekte oluşan ışınımın enerjisi çok sayıda etkileşmeyle taşınır; aynı özgün fotonun kimliğini koruyarak yüzeye çıktığı ima edilmez. γ izi, saçılma/soğurma/yeniden yayım süreçlerinin öğretici bir temsilidir. Yüzeyden çıkan ışık ayrıca gösterilir; Güneş’in hiç ışık yaymadığı söylenmez.
- Nötrinoların çoğu Güneş’ten ve Dünya’dan etkileşmeden geçer. Gösterilen algılama, seçilmiş nadir bir olaydır. Ekrandaki birkaç parçacık, gerçek akı veya etkileşme olasılığı değildir.
- Dedektör, Güneş nötrinolarına duyarlı genel bir optik dedektör şemasıdır; IceCube dizisi değildir. ν–e saçılmasında geri tepen elektronun ürettiği ışık sensörlere ulaşır. SNO/Super-K türü algılama prensibinden esinlenilmiştir; belli bir tesisin mühendislik çizimi değildir.
- Çekirdekte yaklaşan iki tanecik, nükleer süreçleri işaretleyen soyut bir görsel vurgudur; tam reaksiyon denklemi veya tek bir reaksiyonun bütün ürünleri değildir.
- Zamanlar, parçacık hızları, boyutlar, Dünya–Güneş uzaklığı, ışık yolları ve görünür dalga biçimi ölçekli değildir. Nötrino salınımları bu paragrafın kapsamı dışında bırakılmıştır. Sembol ν genel nötrinoyu temsil eder.

## Kaynaklar

- [NASA: Güneş’in iç yapısında foton ve nötrino taşınımı](https://ntrs.nasa.gov/api/citations/20140006649/downloads/20140006649.pdf).
- [SNO: Güneş nötrinolarının ölçümü](https://sno.phy.queensu.ca/first_results/).
- [SNO+: nötrino–elektron elastik saçılması](https://snoplus.phy.queensu.ca/solar.html).

Tüm görseller bu proje için deterministik kodla çizildi. Harici fotoğraf, stok video, ses veya yapay seslendirme kullanılmadı.

Kaynak: `animation/icecube_samples/render_solar_neutrinos.py` ve `motion_common.py`. Yeniden üretim: `render_solar_neutrinos.py --scale 2`. `--preview` yalnızca kontrol kareleri üretir.

Görsel dil tercihi: hareket ve kompozisyon anlatımın yükünü taşımalı; evrensel sembol ve sayılar kullanılabilir; dile bağlı açıklama yazıları eklenmemeli.
