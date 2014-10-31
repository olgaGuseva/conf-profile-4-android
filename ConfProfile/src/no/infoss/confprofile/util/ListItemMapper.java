package no.infoss.confprofile.util;

import no.infoss.confprofile.profile.data.ListItem;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;

import com.litecoding.classkit.view.HeaderObjectAdapter.HeaderObjectMapper;

public class ListItemMapper implements HeaderObjectMapper<ListItem, String> {
	private Context mCtx;
	
	public ListItemMapper(Context ctx) {
		mCtx = ctx;
	}

	@Override
	public View prepareView(int position, View convertView, ListItem data) {
		int viewId = 0;
		
		LayoutInflater inflater = (LayoutInflater) mCtx.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		boolean viewAlreadyCreated = false;
		
		if(convertView == null) {
			//We don't have a view to convert, but...
			if(data == null || data.getModel() == null || data.getModel().getLayoutId() == 0) {
				//We don't know what kind of view should be created,
				//adapter will create default view
				return null;
			} else {
				convertView = inflater.inflate(data.getModel().getLayoutId(), null);
				viewAlreadyCreated = true;
			}
		}
		
		viewId = convertView.getId();
		if(!viewAlreadyCreated) {
			if((data == null || data.getModel() == null || data.getModel().getLayoutId() == 0)) {
				return null;
			} else if(data.getModel().getLayoutId() != viewId) {
				convertView = inflater.inflate(data.getModel().getLayoutId(), null);
			}
		}
		
		return convertView;
	}
	
	@Override
	public void mapData(int position, View view, ListItem data) {
		data.getModel().bind(view);			
	}
	
	@Override
	public View prepareHeaderView(int position, View convertView, String data) {
		// always create a new view
		return null;
	}

	@Override
	public void mapHeader(int position, View view, String data) {
		setText(view, android.R.id.text1, data);
	}
	
	private TextView setText(View rootView, int id, String text) {
		TextView result = null;
		
		View testView = rootView.findViewById(id);
		if(testView != null && testView instanceof TextView) {
			result = (TextView) testView;
			result.setText(text);
		}
		
		return result;
	}
	
}
