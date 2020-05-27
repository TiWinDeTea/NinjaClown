use crate::RawApi;
use ninja_clown_bot_sys::{nnj_entity, nnj_entity_kind};

#[derive(Clone, Debug, Copy, PartialEq)]
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
    #[inline]
    pub fn kind(&self) -> EntityKind {
        unsafe {
            // # Safety
            // Kind should be a valid EntityKind value at any given time
            // Also, the only way to safely get an Entity is through Entities
            // that should never give an Entity with the EK_NOT_AT_ENTITY value kind.
            EntityKind::from_u32_unchecked(self.0.kind.0)
        }
    }

    #[inline]
    pub fn x(&self) -> f32 {
        self.0.x
    }

    #[inline]
    pub fn y(&self) -> f32 {
        self.0.y
    }

    #[inline]
    pub fn angle(&self) -> f32 {
        self.0.angle
    }

    #[inline]
    pub fn handle(&self) -> usize {
        self.0.handle
    }
}

pub struct Entities(Vec<nnj_entity>);

impl Entities {
    pub fn new(raw: &RawApi) -> Self {
        let entities = unsafe {
            let max_entities = (raw.max_entities.unwrap())();
            let mut entities = Vec::new();
            entities.resize_with(max_entities, nnj_entity::default);
            (raw.entities_scan.unwrap())(raw.ninja_descriptor, entities.as_mut_ptr());
            entities
        };

        Self(entities)
    }

    pub fn get(&self, handle: usize) -> Option<&Entity> {
        self.0.get(handle).and_then(|e| {
            unsafe {
                // # Safety
                // Entity struct repr is marked as "transparent", as such &nnj_entity
                // to &Entity cast is okay. Also, we check kind is not EK_NOT_AN_ENTITY because
                // our EntityKind enum doesn't represent this case. We return None instead.
                if e.kind == nnj_entity_kind::EK_NOT_AN_ENTITY {
                    None
                } else {
                    let entity = &*(e as *const nnj_entity as *const Entity);
                    Some(entity)
                }
            }
        })
    }

    pub fn iter(&self) -> Iter<'_> {
        Iter(self.0.iter())
    }

    pub(crate) fn as_mut_ptr(&mut self) -> *mut nnj_entity {
        self.0.as_mut_ptr()
    }
}

impl<'a> IntoIterator for &'a Entities {
    type Item = &'a Entity;
    type IntoIter = Iter<'a>;

    fn into_iter(self) -> Self::IntoIter {
        self.iter()
    }
}

pub struct Iter<'a>(core::slice::Iter<'a, nnj_entity>);

impl<'a> Iterator for Iter<'a> {
    type Item = &'a Entity;

    fn next(&mut self) -> Option<Self::Item> {
        let e = self.0.next()?;
        unsafe {
            // # Safety
            // Entity struct repr is marked as "transparent", as such &nnj_entity
            // to &Entity cast is okay. Also, we check kind is not EK_NOT_AN_ENTITY because
            // our EntityKind enum doesn't represent this case. We return None instead.
            if e.kind == nnj_entity_kind::EK_NOT_AN_ENTITY {
                self.next()
            } else {
                let entity = &*(e as *const nnj_entity as *const Entity);
                Some(entity)
            }
        }
    }
}
