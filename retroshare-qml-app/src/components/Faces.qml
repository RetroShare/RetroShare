import QtQuick 2.7
import Qt.labs.settings 1.0
import "../" // Needed by ChatCache (where stores generated faces)

Item
{

	id: faces

	property string hash
	property var facesCache: ChatCache.facesCache


	Image
	{
		id: imageAvatar
		width: height
		height: iconSize
		visible: true
	}

	Canvas
	{
		id: canvasAvatar
		width: height
		height: canvasSizes
		visible: false

		renderStrategy: Canvas.Threaded;
		renderTarget: Canvas.Image;

		property var images
		property var callback


		onPaint:
		{
			var ctx = getContext("2d");

			if (images)
			{
				for (y = 0 ;  y< nPieces ; y++)
				{
					ctx.drawImage(images[y], 0, 0, iconSize, iconSize )
				}
			}

		}

		onPainted:
		{
			if (callback)
			{
				var data = toDataURL('image/png')
				callback(data)
			}
		}
	}

	Component.onCompleted:
	{
		createFromHex(hash)
	}


	property var facesPath: "/icons/faces/"

	property var iconSize: 32
	property var canvasSizes: iconSize > 32 ? 64 : 32;

	property var nPieces: 6
	property var pieces: []

	// Number of image files corresponding to heach part:
	// [background, face, hair|head, mouth, clothes, eye ]
	property var total:
	({
		female: [5, 4, 33, 17, 59, 53],
		male: [5, 4, 36, 26, 65, 32]
	})

	function src (gender, piece, random)
	{
		var head = gender === 'female' ? 'head' : 'hair';
		var pieces = ['background', 'face', head, 'mouth', 'clothes', 'eye'];
		var num = random % total[gender][piece] + 1;
		return facesPath+gender+canvasSizes+'/'+pieces[piece]+num+'.png';
	}

	function calcDataFromFingerprint(dataHex)
	{
		var females = total.female.reduce(function(previousValue, currentValue)
		{
			return previousValue * currentValue;
		});
		var males = total.male.reduce(function(previousValue, currentValue)
		{
			return previousValue * currentValue;
		});

		var ret = [];
		var data = parseInt(dataHex, 16) % (females+males);
		if (data - females < 0)
		{
			var i = 0;
			ret = ['female'];
			while (data > 0 || i < total.female.length)
			{
				ret.push(data % total.female[i]);
				data = parseInt(data / total.female[i]);
				i++;
			}
		}
		else
		{
			data = data - females;
			var i = 0;
			ret = ['male'];
			while (data > 0 || i < total.male.length)
			{
				ret.push(data % total.male[i]);
				data = parseInt(data / total.male[i]);
				i++;
			}
		}
		return ret;
	}

	function generateImage(data, callback)
	{
		var gender = data[0];
		var onloads = [];
		for (var i=0; i<nPieces; i++)
		{
			var url = src(gender, i, data[i+1])
			onloads.push(url)
			canvasAvatar.loadImage(url)
		}
		canvasAvatar.images = onloads
		canvasAvatar.callback = callback
		canvasAvatar.requestPaint()
	}

	// Create the identicon
	function createFromHex(dataHex)
	{
		var iconId = [dataHex, iconSize];
		var update = function(data)
		    {
			    // This conditions are for solve a bug on an Lg S3.
			    // On this device the toDataURL() is incompleted.
			    // So for see the complete avatar at least at first execution we'll show the canvas,
			    // instead of the image component.
			    // See issue: https://gitlab.com/angesoc/RetroShare/issues/37
			    if (facesCache.iconCache[iconId])
				{
					imageAvatar.source =  data
					imageAvatar.visible = true
					canvasAvatar.visible =  false

					canvasAvatar.height = 0
					imageAvatar.height = iconSize
				}
				else
				{
					canvasAvatar.visible =  true
					imageAvatar.visible =  false

					canvasAvatar.height = iconSize
					imageAvatar.height = 0
				}

				facesCache.iconCache[iconId] = data;
		    }

		if (facesCache.iconCache.hasOwnProperty(iconId))
		{
			update(facesCache.iconCache[iconId])
		}
		else if(facesCache.callbackCache.hasOwnProperty(iconId))
		{
			facesCache.callbackCache[iconId].push(update)
		}
		else
		{
			var onImageGenerated = function(data)
			    {

				    facesCache.callbackCache[iconId].forEach(function(callback)
					    {
						    callback(data);
					    })
			    }

			facesCache.callbackCache[iconId] = [update];
			if (dataHex)
			{
				generateImage(calcDataFromFingerprint(dataHex), onImageGenerated);
			}
		}
	}
}
