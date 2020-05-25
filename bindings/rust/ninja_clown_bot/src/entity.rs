use ninja_clown_bot_sys::{nnj_api, nnj_entity, nnj_entity_kind};

#[derive(Clone, Debug, Copy)]
#[repr(u32)]
pub enum EntityKind {
    Harmless = nnj_entity_kind::EK_HARMLESS.0,
    Patrol = nnj_entity_kind::EK_PATROL.0,
    Aggressive = nnj_entity_kind::EK_AGGRESSIVE.0,
    Projectile = nnj_entity_kind::EK_PROJECTILE.0,
    Dll = nnj_entity_kind::EK_DLL.0,
}

impl EntityKind {
    /// # Safety
    /// Calling this function with a value not representable by EntityKind
    /// is undefined behavior.
    unsafe fn from_u32_unchecked(v: u32) -> Self {
        std::mem::transmute(v)
    }
}

#[derive(Clone, Debug)]
#[repr(transparent)]
pub struct Entity(nnj_entity);

impl Entity {
    pub fn kind(&self) -> EntityKind {
        unsafe {
            // # Safety
            // Kind should be a valid EntityKind value at any given time
            // Also, the only way to safely get an Entity is through Entities
            // that should never give an Entity with the EK_NOT_AT_ENTITY value kind.
            EntityKind::from_u32_unchecked(self.0.kind.0)
        }
    }

    pub fn x(&self) -> f32 {
        self.0.x
    }

    pub fn y(&self) -> f32 {
        self.0.y
    }

    pub fn angle(&self) -> f32 {
        self.0.angle
    }
}

pub struct Entities(Vec<Entity>);

impl Entities {
    pub fn new(raw: &nnj_api) -> Self {
        let entities = unsafe {
            let max_entities = (raw.max_entities.unwrap())() as usize;
            let mut entities = Vec::new();
            entities.resize_with(max_entities, nnj_entity::default);
            (raw.entities_scan.unwrap())(raw.ninja_descriptor, entities.as_mut_ptr());
            std::mem::transmute(entities)
        };

        Self(entities)
    }

    pub fn as_mut_ptr(&mut self) -> *mut Entity {
        self.0.as_mut_ptr()
    }

    pub fn get(&self, handle: usize) -> Option<&Entity> {
        self.0.get(handle).and_then(|e| {
            if e.0.kind == nnj_entity_kind::EK_NOT_AN_ENTITY {
                None
            } else {
                Some(e)
            }
        })
    }
}

// TODO: add way to iterate over Entities (ignoring EK_NOT_AN_ENTITY entities)
