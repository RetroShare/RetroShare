import QtQuick 2.7

Item
{

	id: faceRoot

	property string hash

	Canvas
	{
		id: faceCanvas
		width: iconSize
		height: iconSize

		property var images


		onPaint:
		{
			var ctx = getContext("2d");

			if (images)
			{
				for (y = 0 ;  y< nPieces ; y++)
				{
					ctx.drawImage(images[y], 0, 0	)
				}
			}
		}
	}

	Component.onCompleted:
	{
		createFromHex(hash)
	}


	property var iconCache: ({})
	property var callbackCache: ({})

	property var facesPath: "/icons/faces/"

	property var iconSize: 32
	property var canvasSize: iconSize > 32 ? 64 : 32;

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
		return facesPath+gender+canvasSize+'/'+pieces[piece]+num+'.png';
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
			onloads.push(src(gender, i, data[i+1]))
			faceCanvas.loadImage(src(gender, i, data[i+1]))
		}
		faceCanvas.images = onloads
		faceCanvas.requestPaint()
	}


	// Create the identicon
	function createFromHex(dataHex)
	{
		var iconId = [dataHex, iconSize];
		var update = function(data)
		    {
			    iconCache[iconId] = data;
//				element.html('<img class="identicon" width='+iconSize+' height='+iconSize+' src="'+data+'">');
		    }
		if (iconCache.hasOwnProperty(iconId))
		{
			update(iconCache[iconId]);
		}
		else if(callbackCache.hasOwnProperty(iconId))
		{
			callbackCache[iconId].push(update);
		}
		else
		{
			var onImageGenerated = function(data)
			    {
				    callbackCache[iconId].forEach(function(callback)
					    {
						    callback(data);
					    })
			    }

		callbackCache[iconId] = [update];
		if (dataHex)
		{
			generateImage(calcDataFromFingerprint(dataHex), onImageGenerated);
		}
	  }
	}
}
