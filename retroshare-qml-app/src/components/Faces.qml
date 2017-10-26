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
	}

	Component.onCompleted: createFromHex(hash)

	/* TODO: Is there a reason why we are using var and not proper type for the
	 * following properties? */

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
		}
		var base64Image = androidImagePicker.b64AvatarGen(onloads, canvasSizes)
		callback("data:image/png;base64,"+base64Image)
	}

	// Create the identicon
	function createFromHex(dataHex)
	{
		var iconId = [dataHex, iconSize];
		var update = function(data)
		{
			imageAvatar.source =  data
			imageAvatar.height = iconSize
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
