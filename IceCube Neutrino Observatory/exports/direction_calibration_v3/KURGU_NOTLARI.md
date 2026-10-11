# Yön, zamanlama ve buz kalibrasyonu — v3

132 saniye, 3840 × 2160, 30 fps, sessiz MP4 ve 1080p önizleme.

İlk sahnenin beş sabit, dağınık algılaması kaldırıldı. Dizide zaten bulunan DOM’lar, ekranda görünen müon izine uzaklıklarına göre seçiliyor. En yakın bölgede DOM’ların %93,5’i; sonraki uzaklık bantlarında %63,2, %18,4 ve %1,9’u ışık algılıyor. Toplam 735 DOM seçili. Parlaklık da mesafeyle azalıyor. Parlama, müonun en yakın yol noktasından geçmesinin ardından temsili ışık yayılma gecikmesiyle başlıyor; kayıtlı noktalar yön çıkarımı boyunca görünür kalıyor.

Bu, ekrandaki uzaklıklara dayalı temsili bir olay gösterimidir; gerçek IceCube verisi veya nicel bir dedektör yanıtı simülasyonu değildir. Uzaklık bantları mantıksal görüntü koordinatlarında 0–15, 15–40, 40–75 ve 75–130 pikseldir; metre cinsinden soğurma uzunluğu ifade etmez.

Yakın plana alınan DOM da iz üzerindeki algılama noktalarından seçildi. 28,5 saniyeden sonraki zamanlama, buz ve LED kalibrasyonu sahneleri v2 ile aynıdır. Önceki etiket kaybolma düzeltmesi, animasyonlu 0.000000001 s ve tek müonlu iki kablo/altı DOM sahnesi korunmuştur.

Kaynak: animation/icecube_samples/render_direction_calibration_v3.py. Yeniden üretim: render_direction_calibration_v3.py --scale 2.
