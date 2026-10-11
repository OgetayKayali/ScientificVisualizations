# Işıkla anlamak — v1

40 saniye, 3840 × 2160, 30 fps, sessiz MP4. Ayrı 1080p önizleme, Türkçe SRT ve timing.json bulunur. Ekranda yazı, sayı veya altyazı yoktur. Ses kaydı sağlanmadığından süreler metnin tahmini okunma temposuna göre hazırlanmıştır.

## Kurgu

- 0–9 s: ışık saydam bir numuneden ve prizmadan geçer. Çıkan tayftaki soğurma çizgileri üç referanstan biriyle eşleşir. Çizgiler temsili; belirli bir elementin gerçek tayfı değildir.
- 9–16 s: Güneş benzeri bir yıldızın kırmızı deve doğru değişimi temsil edilir. Yıldız büyür ve kızarır. Sağdaki biçim-normalize Planck eğrisinin tepesi uzun dalga boylarına kayar. Eğrilerin aynı yüksekliğe normalize edilmesi toplam ışınım gücünün sabit olduğu anlamına gelmez. Evrimsel süre ve boyut oranları ölçekli değildir.
- 16–24 s: ötegezegen yıldız diskinin önünden geçer. Parlaklık grafiğinin düşüşü ve toparlanması, aynı anda çizilen iki diskin geometrik örtüşmesinden hesaplanır. Düşüş yüksekliği görsel okunurluk için büyütülmüştür; düzgün parlaklıklı disk yaklaşımı kullanılır. Bu, geçiş yöntemiyle tespiti temsil eder; doğrudan gezegen fotoğrafı değildir.
- 24–28,5 s: sabit X-ışını kaynağı ve radyografi; kemikler açık tonla gösterilir.
- 28,5–33 s: tomografi kaynağı gövde kesiti etrafında döner; dedektör karşısında kalır. Sağda temsili kesitler oluşur. Anatomi ve cihaz çizimleri öğretici şemadır; klinik görüntü değildir.
- 33–40 s: kapalı mikrodalga fırının kesitinde elektromanyetik dalgalar ve ısınan yemek. Yemek ısındıkça sıcak tonlar ve buhar belirir. Renk değişimi sıcaklık için görsel bir işarettir; yemeğin görünür ışık yaydığı anlamına gelmez.

Sahneler 0,8 saniyelik yumuşak geçişlerle bağlanır. Lacivert/buz mavisi ana palet korunur; tayf, yıldız sıcaklığı ve ısınma için kontrollü sıcak renkler kullanılır. Görünür ışık, X-ışını ve mikrodalganın gerçek dalga boyları çizimde ölçekli değildir.

## Kaynak kontrolü

- ESA, soğurma çizgileri ve kimyasal parmak izi: https://www.esa.int/ESA_Multimedia/Images/2022/08/Absorption_and_emission_spectra_of_various_elements
- NASA, Güneş benzeri yıldızın evrimi: https://science.nasa.gov/resource/the-life-cycle-of-a-sun-like-star-annotated/
- NASA, geçiş yöntemi ve ışık eğrisi: https://science.nasa.gov/exoplanets/whats-a-transit/
- NIBIB, dönen X-ışını kaynağı ve tomografik kesitler: https://www.nibib.nih.gov/science-education/science-topics/computed-tomography-ct
- FDA, mikrodalga ile ısıtma: https://www.fda.gov/radiation-emitting-products/resources-you-radiation-emitting-products/microwave-ovens

Kaynak kod: animation/icecube_samples/render_light_applications.py
Yeniden üretim: render_light_applications.py --scale 2
